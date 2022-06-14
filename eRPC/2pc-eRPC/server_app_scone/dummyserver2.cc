#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <inttypes.h>
#include <vector>
#include <thread>
#include "util/transaction_test_util.h"


#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>


#include "util.h"
#include "message.pb.h"
#include "openssl/crypto.h"

#include "trusted_counter/trusted_counter.h"



#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

std::array<amc::Counter, amc::Counter::max_counters> amc::Counter::counters;
std::atomic<int> amc::Counter::index(-1);
std::atomic<uint64_t> amc::Counter::unstable_period;
std::shared_mutex amc::Counter::_rw_lock;

std::vector<std::thread> amc::Counter::timer_thread;
std::atomic<bool> amc::Counter::keep_working;

std::array<std::atomic<uint64_t>, amc::Counter::max_counters> amc::Counter::stable_counters;

std::atomic<uint64_t> rocksdb::RandomTransactionInserter::random_get;
std::atomic<uint64_t> rocksdb::RandomTransactionInserter2::random_get;
std::atomic<uint64_t> rocksdb::RandomTransactionInserter2::random_put;



std::string kDBPath = "/tmp/rocksdb";


using namespace google::protobuf::io;
google::protobuf::Arena arena;

rocksdb::WriteOptions write_options;
rocksdb::ReadOptions read_options;
rocksdb::TransactionOptions txn_options;

pthread_t parent_tid = -1; 		// parent-thread tid for sending signals

// threads arguments;
struct threadArgs {
	util::socket_fd listening_socket_handle;	// sockets which listens for requests
	util::socket_fd sending_socket_handle;		// socket which replies back to client
	rocksdb::TransactionDB* db = nullptr;		// pointer to the database
};



bool execute_txn_req(const tutorial::ClientMessageReq& client_msg_req, rocksdb::Transaction* txn, util::socket_fd& fd) {
	tutorial::ClientMessageResp _resp;
	tutorial::ClientMessageResp* resp = _resp.New(&arena);
	resp->set_resptype(tutorial::ClientMessageResp::OPERATION);
	resp->set_iserror(false);
	for (auto i = 0; i < client_msg_req.operations_size(); i++) {
		const tutorial::Statement& s = client_msg_req.operations(i);
		if (s.optype() == tutorial::Statement::READ) {
			std::string value;
			std::string* add_string = resp->add_readvalues();
			txn->Get(read_options, std::to_string(s.key()) , &value);
			add_string->assign(value);
		}
		else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
			std::string* add_string = resp->add_readvalues();
			std::string value;
			txn->GetForUpdate(read_options, std::to_string(s.key()) , &value);
			add_string->assign(value);
		}
		else if (s.optype() == tutorial::Statement::WRITE) {
			txn->Put(std::to_string(s.key()), s.value());
		}
		else {
			fprintf(stdout, "other type of request\n");
		}

	}

	// reply back to client
	tutorial::Message proto_msg;
	proto_msg.set_messagetype(tutorial::Message::ClientRespMessage);
	proto_msg.set_allocated_clientrespmsg(resp);
	std::string msg;
	proto_msg.SerializeToString(&msg);

#ifdef DEBUG_SOCKETS
	std::cout << proto_msg.DebugString() << "\n";
#endif

	int sz = msg.length();
	std::unique_ptr<char[]> buf = std::make_unique<char[]>(sz + 4);

	util::convertIntToByteArray(buf.get(), sz);
	::memcpy((buf.get()+4), msg.c_str(), sz);

	if (send(fd, buf.get(), sz + 4, 0) < 0)
		util::err("send data");

	return true;
}



static void threadRun(void* args) {
	struct threadArgs* ptr = reinterpret_cast<struct threadArgs*>(args);
	util::socket_fd csock = ptr->listening_socket_handle;
	util::socket_fd fd = ptr->sending_socket_handle;
	rocksdb::TransactionDB* txn_db = ptr->db;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(util::message_size);
	size_t bytecount = 0;

	std::cout << "[Thread " << std::this_thread::get_id() << " is listening ..\n";



	// transaction handle
	rocksdb::Transaction* txn = nullptr;

	while (true) {
		bytecount = 0;
		if ((bytecount = recv(csock, buffer.get(), util::message_size, 0))  <= 0) { 
			if (bytecount == 0) {
				std::cout << "[Thread " << std::this_thread::get_id()  << "] experiment is done\n";
				break;
			}
			util::err("error receiving data", errno);
		}

		size_t actual_msg_size = util::convertByteArrayToInt(buffer.get());

		if (actual_msg_size >= util::message_size) {
			util::err("allocated buffer is not large enough");
		}

		if (actual_msg_size != (bytecount - 4)) {
			util::err("message size and received message size missmatch");
		}


		tutorial::Message p;
		std::string st(buffer.get() + 4, actual_msg_size);

		p.ParseFromString(st);
		auto msgType = p.messagetype();

#ifdef DEBUG_SOCKETS
		std::cout << "Message received: " << p.DebugString() << "\n";
#endif
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
						util::err("register");
						break;
					}

					if (client_msg_req.tostart()) {
						if (txn != nullptr) {
							util::err("txn is not null");
						}

						txn = txn_db->BeginTransaction(write_options);

					}

					execute_txn_req(client_msg_req, txn, fd);

					if (client_msg_req.tocommit()) {
						if (txn == nullptr) {
							util::err("txn is nullptr and we are commiting ..");
						}
						txn->Commit();
						delete txn;
						txn = nullptr;
					}
					else if (client_msg_req.toabort()) {
						std::cout << "toAbort \n";
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
	}
	delete ptr;

	fprintf(stdout, "shutdown sockets ..\n");
	::shutdown(fd, SHUT_RD);
	::shutdown(csock, SHUT_RD);
	::close(fd);
	::close(csock);
	pthread_kill(parent_tid, SIGUSR1);
	std::cout << "[Thread " << std::this_thread::get_id() << " is closing ..\n";
}

std::vector<std::thread> threads;

void signal_handler(int signal_num) { 
	std::cout << "The interrupt signal is (" << signal_num << "). \n"; 

	for (auto& t : threads) {
		t.join();

	}

	fprintf(stdout, "all threads joined\n");


	// terminate program   
	exit(signal_num);   
} 


int main(int argv, char** argc) {
	signal(SIGUSR1, signal_handler);   
	unsigned long * opensslflags = OPENSSL_ia32cap_loc();
	*opensslflags |= (1UL << 19) | (1UL << 23) | (1UL << 24) | (1UL << 25) | (1UL << 26) | (1UL << 41) | (1UL << 57) | (1UL << 60);

	// open DB
	rocksdb::Options options;
	rocksdb::TransactionDBOptions txn_db_options;
	options.create_if_missing = true;
	rocksdb::TransactionDB* txn_db;

	parent_tid = pthread_self();
	rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
	if (!s.ok()) {
		fprintf(stdout, "%s\n", s.ToString().c_str());
		util::err("cannot open rocksdb");
	}



	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int host_port = 20414;

	util::socket_fd sock = util::tcp_listening_socket_init(host_port, "any");
	util::socket_fd fd = -1;

	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(util::message_size);
	uint64_t txns = 0;



	while (true) {
		size_t bytecount = 0;
		::memset(buffer.get(), '\0', util::message_size);
		util::socket_fd csock = util::tcp_accepting_connection(sock);

		if (csock < 0) {
			fprintf(stdout, "accept failed\n");
			break;
		}
		if ((bytecount = recv(csock, buffer.get(), util::message_size, 0))  <= 0) { 
			if (bytecount == 0) {
				fprintf(stdout, "expriment is done\n");
				break;
			}
			util::err("error receiving data", errno);
		}

		size_t actual_msg_size = util::convertByteArrayToInt(buffer.get());
		if (actual_msg_size != (bytecount - 4)) 
			util::err("message size and received message size missmatch");

		if (actual_msg_size >= util::message_size) {
			util::err("allocated buffer is not large enough");
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
						std::cout << "[Main thread " << std::this_thread::get_id() << "] received ClientMessageReq for registering a new client\n";

						tutorial::ClientMessageResp _resp;
						tutorial::ClientMessageResp* resp = _resp.New(&arena);
						resp->set_resptype(tutorial::ClientMessageResp::REGISTER);
						resp->set_iserror(false);


						tutorial::Message proto_msg;
						proto_msg.set_messagetype(tutorial::Message::ClientRespMessage);
						proto_msg.set_allocated_clientrespmsg(resp);

						std::string msg;
						proto_msg.SerializeToString(&msg);

						std::unique_ptr<char[]> buf = std::make_unique<char[]>(msg.length() + 4);
						int sz = msg.length();

						util::convertIntToByteArray(buf.get(), sz);
						::memcpy((buf.get()+4), msg.c_str(), sz);

						fd = util::tcp_sending_socket_init(client_msg_req.clientport(), "any");
						int ret = 0;
						if ((ret = send(fd, buf.get(), sz + 4, 0)) < 0)
							fprintf(stderr, "error in send..\n");

						std::cout << "Send reply for registering client and start thread \n";
						struct threadArgs* args = new threadArgs();
						args->listening_socket_handle = csock;
						args->sending_socket_handle = fd;
						args->db = txn_db;
						threads.push_back(std::thread(threadRun, args));
					}
					else if (client_msg_req.tostart()) {
						util::err("tostart");
					}
					if (client_msg_req.tocommit()) {
						util::err("tocommit");
					}
					else if (client_msg_req.toabort()) {
						util::err("toabort");
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
	}


	return 0;
}
