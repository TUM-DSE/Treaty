#include <vector>
#include <iostream>
#include <map>

#include "app_context.h"
#include "fiber_args.h"

#include "lib/coordinator.h"


// @dimitra one copy of threads_args per application file
static std::map<int, void*> threads_args;
static std::atomic<uint64_t> fiber_count = 0;
static std::atomic<int> txn_count{0};




static uint64_t execute_txn_req(const tutorial::ClientMessageReq& client_msg_req, 
    TCoordinator* txn, util::socket_fd& fd, void* stats, bool& res, int fiber_id) {
	
    // @dimitra one protobuf::Arena per request -- otherwise memory will kill you w/ SCONE
    google::protobuf::Arena arena;
	tutorial::ClientMessageResp _resp;
	tutorial::ClientMessageResp* resp = _resp.New(&arena);


	// string representation of message to be sent 'client_msg_req.DebugString()'
	resp->set_resptype(tutorial::ClientMessageResp::OPERATION);
	resp->set_iserror(false);
	bool error_found = false;

	int read_reqs = 0;
	bool status;

	for (auto i = 0; i < client_msg_req.operations_size(); i++) {
		const tutorial::Statement& s = client_msg_req.operations(i);
		if (s.optype() == tutorial::Statement::READ) {
			read_reqs++;
			std::string value("");

			int ret = txn->Read(std::to_string(s.key()), value, fiber_id);
			if (ret != 2) {
				if (ret != -1) {
					txn->_c->fibers_txn_data[fiber_id]->read_values.insert({txn->GlobalTxn->readIndex() - 1, value});
				}
				else {
					resp->set_iserror(true);
					error_found = true;
					txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
				}
			}
		}
		else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
			read_reqs++;
			std::string value("");

			int ret = txn->ReadForUpdate(std::to_string(s.key()), value, fiber_id);
			if (ret != 2) {
				if (ret != -1) {
					txn->_c->fibers_txn_data[fiber_id]->read_values.insert({txn->GlobalTxn->readIndex() - 1, value});
				}
				else {
					txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
					resp->set_iserror(true);
					error_found = true;
				}
			}
		}
		else if (s.optype() == tutorial::Statement::WRITE) {
			status = txn->Put(std::to_string(s.key()), s.value(), fiber_id);

			if (!status) {
				resp->set_iserror(true);
				error_found = true;
				txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
			} 
		}
		// else if (s.optype() == tutorial::Statement::DELETE) 
		else if (s.Type_Name(s.optype()) == "DELETE") {
			status = txn->Delete(std::to_string(s.key()), fiber_id);
			if (!status) {
				txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
				resp->set_iserror(true);
				error_found = true;
			}
		}
		else {
            std::cerr << " client sent another type of request .. " << s.Type_Name(s.optype()) << "\n";
		}

		if (error_found) 
			break;
	}


    // @dimitra we should wait until all operations of this txn are executed before proceeding
	txn->wait_for_all();

    // copy the read values into the response buffer
	for (auto& elem : txn->_c->fibers_txn_data[fiber_id]->read_values) {
		std::string* add_string = resp->add_readvalues();
		add_string->assign(elem.second);
	}


	if (read_reqs != txn->_c->fibers_txn_data[fiber_id]->read_values.size()) {
		if (txn->_c->fibers_txn_data[fiber_id]->txn_state != 0)
			std::cerr << "read_reqs and read_values do not match while txn_state seems correct ..\n";
	}

	txn->_c->fibers_txn_data[fiber_id]->read_values.clear();
	error_found = !(txn->_c->fibers_txn_data[fiber_id]->txn_state);

	if (client_msg_req.tocommit() && !error_found) {
		
		status = txn->Prepare(fiber_id);
		if (status) 
			status = txn->CommitTransaction(fiber_id);
		else {
			// should rollback
			error_found = true;
			resp->set_iserror(true);
		}
		

		if (!status) {
			std::cerr << "txn " << txn->GlobalTxn->gettxnId() << " failed (during prepare/commit)..\n";
			resp->set_iserror(true);
		}
	}

	if (error_found) {
		resp->set_iserror(true);
		txn->Rollback(fiber_id);
	}

	if (client_msg_req.toabort()) {
		std::cout << "client told us to abort this transaction ..\n";
		txn->Rollback(fiber_id);
	}

	res = error_found;

	// reply back to client
	tutorial::Message proto_msg;
	proto_msg.set_messagetype(tutorial::Message::ClientRespMessage);
	proto_msg.set_allocated_clientrespmsg(resp);
	std::string msg;
	proto_msg.SerializeToString(&msg);

	
    // construct and send the reply back to client
	int sz = msg.length();
	std::unique_ptr<char[]> buf = std::make_unique<char[]>(sz + 4);

	util::convertIntToByteArray(buf.get(), sz);
	::memcpy((buf.get()+4), msg.c_str(), sz);

	size_t sent_bytes = 0;
	if ((sent_bytes = send(fd, buf.get(), sz + 4, 0)) < 0)
		util::err("send data");
	else if (sent_bytes < (sz+4)) {
		auto rem_bytes = (sz+4) - sent_bytes;
		auto ptr = buf.get() + sent_bytes;
		while (rem_bytes > 0) {
			sent_bytes = send(fd, ptr, rem_bytes, 0);
			if (sent_bytes < 0)
				util::err("send data");
			rem_bytes -= sent_bytes;
			ptr += sent_bytes;
		}
	}
	return (sz+4);
}




static void coordinator_fiber(boost_fiber_args* fiber_context) {
	struct threadArgs_boost_fibers* ptr = reinterpret_cast< struct threadArgs_boost_fibers*>(threads_args[fiber_context->index]);

    // if no connections yet, wait on cv
	auto nb_connections = 0;
	while (nb_connections == 0) {
		{
			std::unique_lock<boost::fibers::mutex> lk(ptr->mtx);
			nb_connections = ptr->listening_sockets.size();
			if (nb_connections == 0)   
				(ptr->cv).wait(lk); // this will pass control to another fiber
		}
	}

    std::cout << "thread id " << std::this_thread::get_id() << " [w/ fiber id " << boost::this_fiber::get_id() <<  " (" << fiber_context->fiber_id << ")] has woken up ..\n";
	

	std::map<util::socket_fd, util::socket_fd>& map_fd = ptr->sending_sockets;
	rocksdb::TransactionDB* txn_db = ptr->db;
    std::map<util::socket_fd, TCoordinator*> map_txn;
    
    // @dimitra: we alloc a big message buffer for getting the reqs .. 
    // however, allocation is done once at the beginning .. massif tool didn't complain about this
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(util::message_size);

	int64_t bytecount = 0;
	fiber_count.fetch_add(1); 

	while (true) {
		for (auto csock : ptr->listening_sockets) { // we only server 1 socket/fiber so this is needless
			bytecount = 0;
			while ((bytecount = recv(csock, buffer.get(), util::message_size, 0))  <= 0) { 
				if (bytecount == 0) {

                    // close socket
					if (shutdown(csock, SHUT_WR) <0 -1)
						std::cerr << "failed to shutdown the socket " << std::strerror(errno) << " ..\n";
					::close(csock);

                    // remove socket fd from the map
					auto it = std::find(ptr->listening_sockets.begin(), ptr->listening_sockets.end(), csock);
					ptr->listening_sockets.erase(it);
					
                    // remove the current txn (if any)
                    auto& txn = map_txn[csock];
					if (txn != nullptr)
						delete txn;
					map_txn.erase(csock);

                    // shutdown and remove the listening socket
					auto s_fd = map_fd[csock];
					if (shutdown(s_fd, SHUT_WR) == -1)
						std::cout << "failed to shutdown the socket " << std::strerror(errno) << "\n";
					::close(s_fd);
					map_fd.erase(csock);


					auto id = fiber_count.fetch_sub(1);
                    std::cout << "thread id " << std::this_thread::get_id() << "\
                        [w/ fiber id " << boost::this_fiber::get_id() <<  " (" << fiber_context->fiber_id << ")\
                        and id = " << id << "] (csock " << csock << ", s_fs " << s_fd << ")\n";
	
					// fprintf(stdout, "Thread id: %" PRIu64 " [fiber id: %" PRIu64 " (id == %d)] with csock %d and s_fs %d \n", std::this_thread::get_id(), boost::this_fiber::get_id(), id, csock, s_fd);
					auto nb_connections = 0;
					while (nb_connections == 0) {
						{
							std::unique_lock<boost::fibers::mutex> lk(ptr->mtx);
							nb_connections = ptr->listening_sockets.size();
							if (nb_connections == 0)
								(ptr->cv).wait(lk);
						}
					}

                    // fiber has woken up
					csock = ptr->listening_sockets[0];
					fiber_count.fetch_add(1);
					
                    std::cout << ">> thread id " << std::this_thread::get_id() << " [w/ fiber id " << boost::this_fiber::get_id() <<  " (" << fiber_context->fiber_id << ")] has woken up ..\n";
				}
				else {
					boost::this_fiber::yield();
					continue;
				}
			}

			size_t actual_msg_size = util::convertByteArrayToInt(buffer.get());

			if (actual_msg_size >= util::message_size) {
				util::err("allocated buffer is not large enough");
			}

			if (bytecount > (actual_msg_size + 4)) {
				std::cout << actual_msg_size << " " << bytecount << "\n";
				util::err("we received more bytes .. tcp is a stream..");
			}

			if (actual_msg_size != (bytecount - 4)) {
				int64_t bytes = bytecount - 4;
				int64_t rem_bytes = actual_msg_size - bytes;
				auto ptr = buffer.get() + bytes + 4;
				while (rem_bytes > 0) {
					bytes = recv(csock, ptr, rem_bytes, 0);
					if (bytes < 0) {
						boost::this_fiber::yield();
						continue;
					}
					else {
						if (bytes > rem_bytes) {
							std::cerr << " recv returned " << bytes << " bytes but the remaining bytes for this message are only " << rem_bytes << "\n";
							if (bytes < 0)
								std::cout << " received negative num " << bytes << " (remaining bytes for this message are only " << rem_bytes << ")\n";

							util::err("error receiving (more) data", errno);
						}

						ptr += bytes;
						rem_bytes -= bytes;
					}
				}
			}

			tutorial::Message p;
			std::string st(buffer.get() + 4, actual_msg_size);

			p.ParseFromString(st);
			auto msgType = p.messagetype();
			switch (msgType) {
				case tutorial::Message::HelloMessage:
					util::err("HelloMessage");
					break;
				case tutorial::Message::GoodbyeMessage:
					util::err("GoodbyeMessage");
					break;
				case tutorial::Message::ClientReqMessage:
					{
						const tutorial::ClientMessageReq& client_msg_req = p.clientreqmsg();
						if (client_msg_req.register_()) {
							std::cout << "we received invalid message (tutorial::Message::ClientReqMessage::REGISTER \n" << p.DebugString() << "\n";
							util::err("register");
							break;
						}

						if (client_msg_req.tostart()) {
							fiber_context->context->fibers_txn_data[fiber_context->fiber_id]->txn_state = 1;
							fiber_context->context->fibers_txn_data[fiber_context->fiber_id]->read_values.clear();
							txn_count.fetch_add(1);
							
							auto txn = (map_txn.find(csock) != map_txn.end()) ? map_txn.find(csock)->second : nullptr;
							if (txn != nullptr) {
								delete txn;
								txn = nullptr;
							}


                            // create a new TCoordinator object 
							auto tc =  new TCoordinator(fiber_context->context);
							
							if (map_txn.find(csock) != map_txn.end()) {
								map_txn.find(csock)->second = tc;
							}
							else {
								map_txn.insert({csock, tc});
							}
				    	}

					    auto _fd = map_fd[csock];
					    auto txn = map_txn[csock];

					    auto _stats = nullptr;
					    bool res;
					    uint64_t ret = execute_txn_req(client_msg_req, txn, _fd, _stats, res, fiber_context->fiber_id);
                        
                        if (client_msg_req.tocommit()) {
                            auto& txn = map_txn.find(csock)->second;
                            if (txn == nullptr) {
                                util::err("txn is nullptr and we are commiting ..");
                            }
                            // execute txn will have alreday committed the transaction if successfull
                            delete txn;
                            txn = nullptr;
                        }
                        else if (client_msg_req.toabort()) {
                            auto& txn = map_txn.find(csock)->second;
                            std::cout << " [SOS] .. we need to abort \n";
                            
                            delete txn;
                            txn = nullptr;
                        }
                        else if (!ret) {
                            auto& txn = map_txn.find(csock)->second;
                            delete txn;
                            txn = nullptr;
                        }
			        }
			        break;
			    case tutorial::Message::ClientRespMessage:
			        util::err("ClientRespMessage");
			        break;
			    case tutorial::Message::DataReqMessage:
			        util::err("DataReqMessage");
			        break;
			    case tutorial::Message::DataRespMessage:
			        util::err("DataRespMessage");
			        break;
			    default:
			        break;
		    }
		
		    boost::this_fiber::yield();
	    }
    }
}



void coordinator(int* ptr, erpc::Nexus* nexus, AppContext* context) {

	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), context->RID,  sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;


    auto it = context->remote_nodes.begin();
    if (context->remote_nodes.size() != (context->cluster_size-1)) {
        std::cerr << "cluster size does not match the configured nodes ..\n";
        exit(-1);
    }

    for (int i = 0; i < (context->cluster_size-1); i++) {
        std::string uri = it->hostname + ":" + std::to_string(kUDPPort);
        int session_num = context->rpc->create_session(uri, *ptr);
        while (!context->rpc->is_connected(session_num)) context->rpc->run_event_loop_once();
        std::cout << "coordinator thread connected to " << uri << " w/ remote RPC id: " << (*ptr) << "\n";
        context->cluster_map[it->node_id].session_num = session_num;
        it++;
    }

    
   
	std::cout << "2pc w/ eRPC: all sessions are now connected ...\n";

	sleep(1); // sleep for 1 sec to give time to connections

	auto thread_id = context->RID;
	std::cout << "thread #" << thread_id << " is ready to run ..\n";
	
	std::vector<boost::fibers::fiber> fibers; 
	std::vector<boost_fiber_args*> fiber_args;
	int starting_index = thread_id;
	for (int i = 0; i < context->nb_fibers; i++) {
		auto ptr = new boost_fiber_args();
		ptr->context = context;
		
		ptr->index = starting_index;
		ptr->fiber_id = (i+1);

        // TODO: need to check the size of the array there
		ptr->context->fibers_txn_data[(i+1)] = new AppContext::transaction_info_per_fiber();
		starting_index += context->nb_worker_threads;
		fiber_args.push_back(ptr);
	}

	for (int i = 0; i < context->nb_fibers; i++) {
		fibers.emplace_back(boost::fibers::launch::post, coordinator_fiber, fiber_args[i]);
	}

	std::cout << "fibers launched .. good luck!\n";

	for (auto& fiber : fibers)
		fiber.join();

	std::cout << "all fibers joined .. \n";

}
