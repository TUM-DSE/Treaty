#include <thread>


#include "tpcc_trace_parser.h"
#include "participant_thread.h"
#include "coordinator_thread.h"

#include "common_conf.h"
#include "recovery/recover.h"
#include "sample_operations.h"
#include "txn.h"
#include "coordinator.h"

#include "transactions_info.h"

#include "request_handlers.h"
#include "termination.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include "args_parser/args_parser.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <boost/fiber/all.hpp>

#ifdef SCONE
#include "server_app_scone/util.h"
#include "server_app_scone/message.pb.h"
#include "memtable/speicher/skiplist.h"
#include <rocksdb_merkle_types.h>
#include <cipher.h>
#include <cipher_ssl.h>
#else
#include "server_app/util.h"
#include "server_app/message.pb.h"
#endif


#include <gflags/gflags.h>

using GFLAGS_NAMESPACE::ParseCommandLineFlags;
using GFLAGS_NAMESPACE::RegisterFlagValidator;
using GFLAGS_NAMESPACE::SetUsageMessage;


#ifdef SCONE
std::array<amc::Counter, amc::Counter::max_counters> amc::Counter::counters;
std::atomic<int> amc::Counter::index(-1);
std::atomic<uint64_t> amc::Counter::unstable_period;
std::shared_mutex amc::Counter::_rw_lock;

std::vector<std::thread> amc::Counter::timer_thread;
std::atomic<bool> amc::Counter::keep_working;
volatile std::atomic<int> amc::Counter::iterations(0);
std::array<std::atomic<uint64_t>, amc::Counter::max_counters> amc::Counter::stable_counters;
#endif

using namespace google::protobuf::io;

/*
std::map<int, void*> threads_args;
std::atomic<uint64_t> fiber_count = 0;
*/

rocksdb::Options options;
static rocksdb::WriteOptions write_options;
rocksdb::TransactionDBOptions txn_db_options;
rocksdb::TransactionDB* txn_db;
static rocksdb::Env* FLAGS_env = rocksdb::Env::Default();

// std::string kDBPath = "/tmp/rocksdb_donna";

/*
// forward declaration
void coordinator(int* ptr, erpc::Nexus*, AppContext*);
void participant(int* ptr, erpc::Nexus*, AppContext*);
*/

CTSL::HashMap<std::string, erpc::MsgBuffer> Txn::pool_resp; 
CTSL::HashMap<std::string, std::string> Txn::index_table; 

std::atomic<int> Txn::txn_ids;
std::atomic<int> thread_count{0};
/*
std::atomic<int> txn_count{0};
*/

extern std::shared_ptr<PacketSsl> txn_cipher;

/*
static int coordinators_num = 0;
static int transactions_num = 2000;

constexpr int FLAGS_nb_worker_threads = 1;
constexpr int FLAGS_nb_fibers = 20;
constexpr int FLAGS_cpu_cores = 2;
constexpr int cluster_size = 3;
constexpr int current_node_id = 0;
constexpr int FLAGS_nb_trace_files = 661142;
constexpr int FLAGS_nb_trace_files = 141121;

std::string FLAGS_traces_filename_read =  "loader_traces_2W/test_";
*/
DEFINE_int32(nb_worker_threads, 1, " ");
DEFINE_int32(nb_fibers, 20, " ");
DEFINE_int32(cpu_cores, 2, " ");
DEFINE_int32(cluster_size, 3, " ");
DEFINE_int32(current_node_id, 0, " ");
DEFINE_bool(use_loader, false, " ");
DEFINE_bool(use_merger, false, " ");

DEFINE_string(traces_filename_read, "loader_traces_2W/test_", " ");
DEFINE_string(path_of_merged_files, "/scratch/dimitra/tmp", " ");
DEFINE_int32(nb_trace_files, 141121, "..");
DEFINE_string(kDBPath, "/tmp/rocksdb_donna", " ");

void print_input_options() {
	std::cout << "worker threads (#coordinators) \: " << FLAGS_nb_worker_threads <<" \n";
	std::cout << "nb_fibers (#clients)           \: " << FLAGS_nb_fibers <<" \n";
	std::cout << "nb_cores (#cpus)               \: " << FLAGS_cpu_cores <<" \n";
	std::cout << "current_node_id (#cpus)        \: " << FLAGS_current_node_id <<" \n";
#ifdef ENCRYPTION
	std::cout << "Network layer					\: w/ Encryption\n";
#else
	std::cout << "Network layer					\: w/o Encryption\n";
#endif
}


#if 0
// code for loader
namespace trace_parser {
	bool should_load(std::string const& key) {
		auto id = std::hash<std::string>{}(key)%(FLAGS_cluster_size);
		return id == FLAGS_current_node_id;
	}

	struct loader_threads_args {
		std::string filename;
		rocksdb::TransactionDB* txn_db;
		int id;
	};

	void loader_func(void* args) {
		auto th_args = reinterpret_cast<struct loader_threads_args*>(args);

		std::string _filename = th_args->filename;
		std::string filename = th_args->filename;
		auto txn_db = th_args->txn_db;
		auto id = th_args->id; 
		auto range = (FLAGS_nb_trace_files) / FLAGS_cpu_cores;
		auto start = (id * range);
		auto end = (id == 7) ? FLAGS_nb_trace_files : ((id + 1) * range);

		uint64_t total_txns = 0;
		rocksdb::Transaction* txn = nullptr;
		rocksdb::Status _status;

		for (int i = start; i < end; i++) {
			filename += std::to_string(i);
			int fileDescriptor = open(filename.c_str(), O_RDONLY);
			google::protobuf::io::FileInputStream fileInput(fileDescriptor);
			fileInput.SetCloseOnDelete( true );
			tutorial::ClientMessageReq client_msg_req;
			if (google::protobuf::TextFormat::Parse(&fileInput, &client_msg_req))
			{
				std::cout << "Read Input File - " << filename << std::endl;
				//std::cout << tasking.DebugString() << "\n";
			}
			else {
				std::cerr << "Error Read Input File - " << filename << std::endl;
				// exit(-1);
				// break;
			}

			if (txn != nullptr) {
				std::cout << "should be nullptr (non-committed txn)\n";
				exit(128);
			}
			bool loaded_data = false;
			txn = txn_db->BeginTransaction(write_options);
			for (auto i = 0; i < client_msg_req.operations_size(); i++) {
				const tutorial::Statement& s = client_msg_req.operations(i);
				if (s.optype() == tutorial::Statement::READ) {
					// fprintf(stdout, "READ\n");
					continue;
				}
				else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::WRITE) {
					if (should_load(std::to_string(s.key()))) {
						loaded_data = true;
						_status = txn->Put(std::to_string(s.key()), s.value());
						if (!_status.ok()) {
							fprintf(stdout, "Transaction::Put %s -- %s\n", _status.ToString().c_str(), std::to_string(s.key()).c_str());
						}
					}
				}
				else{                                                                                                                                                                                                        fprintf(stdout, "other type of request ---> %s\n", s.Type_Name(s.optype()).c_str());

				}
			}                                                                                                                                                 
			if (client_msg_req.tocommit() && loaded_data) {
				_status = txn->Commit();
				// TODO: potential leak here!!!
				delete txn;
				txn = nullptr;
				total_txns += 1;

				if (!_status.ok()) {
					fprintf(stdout, "Transaction::Commit %s\n", _status.ToString().c_str());
				}
			}
			if (!loaded_data) {
				delete txn;
				txn = nullptr;
			}
			filename = _filename;
		}
		fprintf(stdout, "total txns = %" PRIu64 " from %d to %d\n", total_txns, start, end);
	}


	void multithreaded_loader_workload2(std::string filename, rocksdb::TransactionDB* txn_db) {
		std::vector<std::thread> loader_threads;
		std::vector<std::unique_ptr<struct loader_threads_args>> loader_args;

		for (int i = 0; i < FLAGS_cpu_cores; i++) {
			auto ptr = std::make_unique<struct loader_threads_args>();
			ptr->id = i;
			ptr->filename = filename;
			ptr->txn_db = txn_db;
			loader_args.emplace_back(std::move(ptr));
			loader_threads.emplace_back(std::thread(loader_func, loader_args.back().get()));
		}


		for (auto& t : loader_threads) {
			t.join();
		}
	}

	void loader_workload2(std::string filename, rocksdb::TransactionDB* txn_db) {
		std::string _filename = filename;

		uint64_t total_txns = 0;
		rocksdb::Transaction* txn = nullptr;
		rocksdb::Status _status;
		for (int i = 0; i < FLAGS_nb_trace_files; i++) {
			filename += std::to_string(i);
			int fileDescriptor = open(filename.c_str(), O_RDONLY);
			google::protobuf::io::FileInputStream fileInput(fileDescriptor);
			fileInput.SetCloseOnDelete( true );
			tutorial::ClientMessageReq client_msg_req;
			if (google::protobuf::TextFormat::Parse(&fileInput, &client_msg_req))
			{
				std::cerr << "Read Input File - " << filename << std::endl;
				//std::cout << tasking.DebugString() << "\n";
			}
			else {
				std::cerr << "Error Read Input File - " << filename << std::endl;
				// exit(-1);
				// break;
			}

			if (txn != nullptr) {
				std::cout << "should be nullptr (non-committed txn)\n";
				exit(128);
			}
			bool loaded_data = false;
			txn = txn_db->BeginTransaction(write_options);
			for (auto i = 0; i < client_msg_req.operations_size(); i++) {
				const tutorial::Statement& s = client_msg_req.operations(i);
				if (s.optype() == tutorial::Statement::READ) {
					// fprintf(stdout, "READ\n");
					continue;
				}
				else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::WRITE) {
					if (should_load(std::to_string(s.key()))) {
						loaded_data = true;
						_status = txn->Put(std::to_string(s.key()), s.value());
						if (!_status.ok()) {
							fprintf(stdout, "Transaction::Put %s -- %s\n", _status.ToString().c_str(), std::to_string(s.key()).c_str());
						}
					}
				}
				else{                                                                                                                                                                                                        fprintf(stdout, "other type of request ---> %s\n", s.Type_Name(s.optype()).c_str());
				}	}                                                                                                                                                 
			if (client_msg_req.tocommit() && loaded_data) {
				_status = txn->Commit();
				// TODO: potential leak here!!!
				delete txn;
				txn = nullptr;
				total_txns += 1;

				if (!_status.ok()) {
					fprintf(stdout, "Transaction::Commit %s\n", _status.ToString().c_str());
				}
			}
			if (!loaded_data) {
				delete txn;
				txn = nullptr;
			}
			filename = _filename;
		}
		fprintf(stdout, "total txns = %" PRIu64 "\n", total_txns);
	}
};
// end code of loader
#endif


#if 0
struct threadArgs_boost_fibers {
	std::vector<util::socket_fd> listening_sockets; // sockets which listens for requests
	std::map<util::socket_fd, util::socket_fd> sending_sockets;             // socket which replies back to client
	rocksdb::TransactionDB* db = nullptr;           // pointer to the database
	boost::fibers::mutex mtx;
	boost::fibers::condition_variable cv;
	std::string remote_ip;

};
#endif

#if 0
struct boost_fiber_args {
	std::vector<util::socket_fd> l_sockets;
	std::map<util::socket_fd, util::socket_fd> s_sockets;
	rocksdb::TransactionDB* db = nullptr;           // pointer to the database
	int index = -1;
	int fiber_id = -1;
	AppContext* context;
};
#endif


static void Server1(erpc::Nexus* nexus) {
	std::vector<AppContext*> coordinator_context;
	std::vector<AppContext*> participant_context;

	google::protobuf::Arena arena;              
	thread_count.store(0);  
	uint64_t nb_sockets = 0;

	GOOGLE_PROTOBUF_VERIFY_VERSION;


	std::cout << "spawn the threads ..\n";   
	std::vector<std::thread> threads;
	for (size_t i = 0; i < (FLAGS_nb_worker_threads*FLAGS_nb_fibers); i++) {
		auto ptr = new threadArgs_boost_fibers();
		threads_args.insert({i, ptr});
	}

	for (size_t i = 0; i < FLAGS_nb_worker_threads; i++) {
		auto context = new AppContext();
		context->rocksdb_ptr = txn_db;
		auto ptr = new int(FLAGS_cpu_cores -i -1);
		context->RID = i;
		context->cluster_size = FLAGS_cluster_size;
		context->node_id = FLAGS_current_node_id;
		context->nb_fibers = FLAGS_nb_fibers;
		context->nb_worker_threads = FLAGS_nb_worker_threads;
		context->remote_nodes.push_back(AppContext::Nodes_info{1, kmarthaHostname});
		context->remote_nodes.push_back(AppContext::Nodes_info{2, kroseHostname});
		coordinator_context.push_back(context);
		threads.push_back(std::thread(coordinator, ptr, nexus, coordinator_context.back()));
	}

	for (size_t i = 0; i < FLAGS_nb_worker_threads; i++) {
		auto context = new AppContext();
		context->rocksdb_ptr = txn_db;
		auto ptr = new int(FLAGS_cpu_cores - i - 1);
		context->RID = (FLAGS_cpu_cores - i - 1);
		context->node_id = FLAGS_current_node_id;
		context->nb_fibers = FLAGS_nb_fibers;

		participant_context.push_back(context);
		threads.push_back(std::thread(participant, ptr, nexus, participant_context.back()));

	}

	int host_port = 20414;

	util::socket_fd sock = util::tcp_listening_socket_init(host_port, "any");
	util::socket_fd fd = -1;

	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(util::message_size);
	uint64_t txns = 0;
	bool terminate_msg = false;


	tutorial::Message termination_msg;
	while (!terminate_msg) {
		size_t bytecount = 0;
		::memset(buffer.get(), '\0', util::message_size);
		std::string client_ip;
		util::socket_fd csock = util::tcp_accepting_connection(sock, client_ip);


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

						fd = util::tcp_sending_socket_init(client_msg_req.clientport(), client_ip);
						int ret = 0;
						if ((ret = send(fd, buf.get(), sz + 4, 0)) < 0)
							fprintf(stderr, "error in send..\n");

						auto thread_id = nb_sockets % (FLAGS_nb_worker_threads*FLAGS_nb_fibers);
						nb_sockets++;

						std::cout << "Send reply for registering client and update thread's fields [thread : " << thread_id << "]\n";
						if (thread_id > FLAGS_nb_worker_threads*FLAGS_nb_fibers) {
							std::cout << thread_id << "\n";
							exit(2);
						}
						{ 
							std::lock_guard<boost::fibers::mutex> temp(reinterpret_cast<struct threadArgs_boost_fibers*>(threads_args[thread_id])->mtx);
							fcntl(csock, F_SETFL, O_NONBLOCK);
							reinterpret_cast<struct threadArgs_boost_fibers*>(threads_args[thread_id])->sending_sockets.insert({csock, fd});
							reinterpret_cast<struct threadArgs_boost_fibers*>(threads_args[thread_id])->listening_sockets.push_back(csock);
							std::cout << "set the dbs ptrs ..\n";
							std::cout << "csock : " << csock << " fd : " << fd << "\n";
							reinterpret_cast<struct threadArgs_boost_fibers*>(threads_args[thread_id])->db = txn_db;
						}
						reinterpret_cast<struct threadArgs_boost_fibers*>(threads_args[thread_id])->cv.notify_one();
					}
					else if (client_msg_req.toterminate()) {
						fprintf(stdout, "main thread received termination message\n");
						terminate_msg = true;
						termination_msg = p;
						break;
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
	const tutorial::ClientMessageReq& client_msg_req = termination_msg.clientreqmsg();
	fprintf(stdout, "main thread received termination message\n");
	tutorial::ClientMessageResp _resp;
	tutorial::ClientMessageResp* resp = _resp.New(&arena);
	resp->set_resptype(tutorial::ClientMessageResp::TERMINATE);
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

	fprintf(stdout, "wait for threads to join\n");

	for (auto& t : threads) {
		t.join();

	}

	fprintf(stdout, "all threads joined\n");

	// delete txn_db;
}


int main(int argc, char* argv[]) {
	/*
	   args_parser::ArgumentParser _args("Treaty");
	   _args.parse_input(argc, argv);
	   coordinators_num = _args.get_num_of_coordinators();
	   transactions_num = _args.get_num_of_txns();
	   */

	ParseCommandLineFlags(&argc, &argv, true);
	uint8_t __key[16] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
	uint8_t __iv[12] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb};
	local_txns = new CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>();

	std::shared_ptr<KeyIV> keyIv = std::make_shared<KeyIV>(reinterpret_cast<std::byte*>(__key), reinterpret_cast<std::byte*>(__iv));
	txn_cipher = std::make_shared<PacketSsl>(keyIv);

	options.create_if_missing = true;
#ifdef SCONE
	constexpr int FLAGS_skip_list_lookahead = 0;
	options.memtable_factory.reset(new speicher::SkipFactory(FLAGS_skip_list_lookahead));
#endif

	unsigned long * opensslflags = OPENSSL_ia32cap_loc();
	*opensslflags |= (1UL << 19) | (1UL << 23) | (1UL << 24) | (1UL << 25) | (1UL << 26) | (1UL << 41) | (1UL << 57) | (1UL << 60);

#ifdef ENCRYPTED_SSTABLES

	// Would be nice to have optional, but this needs further work in table/block_based_table_reader.cc.
#ifdef OPENSSL_ENCRYPTION
	auto key = std::make_shared<treaty_openssl::EncryptionProvider::Key>();
	fprintf(stdout, "Encryption func: treaty_openssl\n");
#else
	auto key = std::make_shared<merkle::EncryptionProvider::Key>();
	fprintf(stdout, "Encryption func: botan\n");
#endif
	uint64_t ___key[] = {0xF153b7701368e02cULL, 0x6ab1f01ef4c7213fULL, 0xa03cf6f4eb430f5dULL, 0x0b5c7c1d7c175e48ULL};
	memcpy(key->mem, ___key, key->Size);
#ifdef OPENSSL_ENCRYPTION
	FLAGS_env = rocksdb::NewEncryptedEnv(FLAGS_env, new treaty_openssl::EncryptionProvider(std::move(key)));
#else
	FLAGS_env = rocksdb::NewEncryptedEnv(FLAGS_env, new merkle::EncryptionProvider(std::move(key)));
#endif
	std::cout << " ----------- \n";
	FLAGS_env->__print();
	std::cout << " ----------- \n";
#endif

	options.env = FLAGS_env;
	rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_db_options, FLAGS_kDBPath, &txn_db);
	assert(s.ok());
	LocalTxn::ptr_db = txn_db;

	signal(SIGINT, ctrl_c_handler);
	std::string server_uri = kdonnaHostname + ":" + std::to_string(kUDPPort);


	erpc::Nexus nexus(server_uri, 0, 0);

	
	nexus.register_req_func(kReqTxnPrepare, 		req_handler_txnPrepare);
	nexus.register_req_func(kReqTxnCommit, 			req_handler_txnCommit);
	nexus.register_req_func(kReqTxnPut, 			req_handler_txnPut);
	nexus.register_req_func(kReqTxnRead, 			req_handler_txnRead);
	nexus.register_req_func(kReqTxnReadForUpdate, 	req_handler_txnReadForUpdate);
	nexus.register_req_func(kReqTxnRollback, 		req_handler_txnRollback);
	nexus.register_req_func(kReqTxnDelete, 			req_handler_txnDelete);

	print_input_options();
	
	// first load the db -- used for TPCC workload
	if (FLAGS_use_merger)
		trace_parser::multithreaded_merger(FLAGS_traces_filename_read, FLAGS_nb_trace_files, txn_db, FLAGS_current_node_id, FLAGS_cluster_size, FLAGS_path_of_merged_files);

	if (FLAGS_use_loader) {
		std::cout << "TPC-C traces filepath " << FLAGS_traces_filename_read <<"\n";
		trace_parser::multithreaded_loader_workload(FLAGS_traces_filename_read, FLAGS_nb_trace_files, txn_db, FLAGS_current_node_id, FLAGS_cluster_size, FLAGS_path_of_merged_files);
		std::cout << "loading completed (" << FLAGS_nb_trace_files <<" files)\n";
	}

	// server 
	Server1(&nexus);


	local_txns->clear();
	delete local_txns;

	// Cleanup
	delete txn_db;
	DestroyDB(FLAGS_kDBPath, options);

	// _args.to_string();
	/*
	std::cout << "commits_requested: " << commits_requested << "\n";
	std::cout << "commits_served: " << commits_served << "\n";
	*/

	return 0;
}


#if 0
static uint64_t execute_txn_req(const tutorial::ClientMessageReq& client_msg_req, TCoordinator* txn, util::socket_fd& fd, void* stats, bool& res, int fiber_id) {
	google::protobuf::Arena arena;
	tutorial::ClientMessageResp _resp;
	tutorial::ClientMessageResp* resp = _resp.New(&arena);


	// std::cout << "Message to be sent: " << client_msg_req.DebugString() << "\n";
	resp->set_resptype(tutorial::ClientMessageResp::OPERATION);
	resp->set_iserror(false);
	bool error_found = false;

	// std::cout << "fiber_id [ " << std::this_thread::get_id() << "]: " << fiber_id << " executes " << client_msg_req.operations_size() <<  " operations\n";

	int read_reqs = 0;
	bool _status;
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
					//          fprintf(stdout, "Transaction::Read()\n");
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
					// fprintf(stdout, "Transaction::GetForUpdate\n");
				}
			}
		}
		else if (s.optype() == tutorial::Statement::WRITE) {
			_status = txn->Put(std::to_string(s.key()), s.value(), fiber_id);

			if (!_status) {
				// fprintf(stdout, "Transaction::Put\n");
				resp->set_iserror(true);
				error_found = true;
				txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
			} 
		}

		// else if (s.optype() == tutorial::Statement::DELETE) 
		else if (s.Type_Name(s.optype()) == "DELETE") {
			_status = txn->Delete(std::to_string(s.key()), fiber_id);
			if (!_status) {
				// fprintf(stdout, "Transaction::Delete\n");
				txn->_c->fibers_txn_data[fiber_id]->txn_state = 0;
				resp->set_iserror(true);
				error_found = true;
			}
		}

		else {
			fprintf(stdout, "other type of request ---> %s\n", s.Type_Name(s.optype()).c_str());
		}
		if (error_found) 
			break;

	}

	// std::cout << "fiber_id: " << fiber_id << " waits for " << client_msg_req.operations_size() <<  " operations\n";
	txn->wait_for_all();
	// std::cout << "fiber_id  [ " << std::this_thread::get_id() << "]:" << fiber_id << " received ack for " << client_msg_req.operations_size() <<  " operations\n";


	for (auto& elem : txn->_c->fibers_txn_data[fiber_id]->read_values) {
		std::string* add_string = resp->add_readvalues();
		add_string->assign(elem.second);
	}


	if (read_reqs != txn->_c->fibers_txn_data[fiber_id]->read_values.size()) {
		if (txn->_c->fibers_txn_data[fiber_id]->txn_state != 0)
			std::cerr << "error_flag should be 0\n";
		//    std::cout << "[Error] read requests with read_values do not match: " << read_reqs << " " << txn->_c->fibers_txn_data[fiber_id]->read_values.size() <<  " (error must be zero :" << (txn->_c->fibers_txn_data[fiber_id]->txn_state) << ")\n";
		/*
		   for (auto& elem : txn->_c->fibers_txn_data[fiber_id]->read_values)
		   std::cout << "found at: " << elem.first << "\n";
		   */

	}


	txn->_c->fibers_txn_data[fiber_id]->read_values.clear();

	error_found = !(txn->_c->fibers_txn_data[fiber_id]->txn_state);

	if (client_msg_req.tocommit() && !error_found) {
		//  std::cout << "fiber_id [ " << std::this_thread::get_id() << "]:" << fiber_id << " wants to commit " << txn->GlobalTxn->gettxnId() << "\n";
		_status = txn->Prepare(fiber_id);
		// std::cout << "fiber_id [ " << std::this_thread::get_id() << "]:" << fiber_id << " prepared " << txn->GlobalTxn->gettxnId() << "\n";
		if (_status) 
			_status = txn->CommitTransaction(fiber_id);
		else {
			// should rollback
			error_found = true;
			resp->set_iserror(true);
		}

		if (!_status) {
			fprintf(stdout, "Transaction::Commit\n");
			resp->set_iserror(true);
		}
		// std::cout << "fiber_id [ " << std::this_thread::get_id() << "]:" << fiber_id << " committed " << txn->GlobalTxn->gettxnId() << "\n";
	}
	if (error_found) {
		// std::cout << "Error found\n";
		resp->set_iserror(true);
		txn->Rollback(fiber_id);
	}
	if (client_msg_req.toabort()) {
		std::cout << "instructed to abort\n";
		txn->Rollback(fiber_id);
	}
	res = error_found;

	// reply back to client
	tutorial::Message proto_msg;
	proto_msg.set_messagetype(tutorial::Message::ClientRespMessage);
	proto_msg.set_allocated_clientrespmsg(resp);
	std::string msg;
	proto_msg.SerializeToString(&msg);

	// std::cout << "fiber_id: " << fiber_id << " replies back\n";
	//std::cout << "Message to be sent: " << proto_msg.DebugString() << "\n";

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
#endif


#if 0
static void fiber_run_func(boost_fiber_args* _ptr) {
	struct threadArgs_boost_fibers* ptr = reinterpret_cast< struct threadArgs_boost_fibers*>(threads_args[_ptr->index]);

	auto nb_connections = 0;
	while (nb_connections == 0) {
		{
			std::unique_lock<boost::fibers::mutex> lk(ptr->mtx);
			nb_connections = ptr->listening_sockets.size();
			if (nb_connections == 0)   
				(ptr->cv).wait(lk); // this will pass control to another fiber
		}
	}                            
	fprintf(stdout, "Thread id: %" PRIu64 " [fiber id: % " PRIu64 "(%d)] has woken up .. \n", std::this_thread::get_id(), boost::this_fiber::get_id(), _ptr->fiber_id);

	std::map<util::socket_fd, util::socket_fd>& map_fd = ptr->sending_sockets;
	rocksdb::TransactionDB* txn_db = ptr->db;
	// rocksdb::OptimisticTransactionDB* opt_txn_db = ptr->opt_txn_db;
	// std::map<util::socket_fd, rocksdb::Transaction*> map_txn;
	std::map<util::socket_fd, TCoordinator*> map_txn;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(util::message_size);

	int64_t bytecount = 0;
	fprintf(stdout, "Thread id: %" PRIu64 " [fiber id: % " PRIu64 "]\n", std::this_thread::get_id(), boost::this_fiber::get_id());

	fiber_count.fetch_add(1); 

	while (true) {
		for (auto csock : ptr->listening_sockets) {
			bytecount = 0;
			while ((bytecount = recv(csock, buffer.get(), util::message_size, 0))  <= 0) { 
				if (bytecount == 0) {

					if (shutdown(csock, SHUT_WR) == -1)
						std::cout << "failed to shutdown the socket " << std::strerror(errno) << "\n";
					::close(csock);
					auto it = std::find(ptr->listening_sockets.begin(), ptr->listening_sockets.end(), csock);
					ptr->listening_sockets.erase(it);
					auto& txn = map_txn[csock];
					if (txn != nullptr)
						delete txn;
					map_txn.erase(csock);

					auto s_fd = map_fd[csock];
					if (shutdown(s_fd, SHUT_WR) == -1)
						std::cout << "failed to shutdown the socket " << std::strerror(errno) << "\n";
					::close(s_fd);
					map_fd.erase(csock);



					auto id = fiber_count.fetch_sub(1);
					fprintf(stdout, "Thread id: %" PRIu64 " [fiber id: %" PRIu64 " (id == %d)] with csock %d and s_fs %d \n", std::this_thread::get_id(), boost::this_fiber::get_id(), id, csock, s_fd);
					auto nb_connections = 0;
					while (nb_connections == 0) {
						{
							std::unique_lock<boost::fibers::mutex> lk(ptr->mtx);
							nb_connections = ptr->listening_sockets.size();
							if (nb_connections == 0)
								(ptr->cv).wait(lk);
						}
					}
					csock = ptr->listening_sockets[0];
					fiber_count.fetch_add(1);

					fprintf(stdout, ">> Thread id: %d [fiber id: %d] has woken up .. \n", std::this_thread::get_id(), boost::this_fiber::get_id());

					// break;
				}
				else {
					// boost::this_fiber::sleep_for(std::chrono::nanoseconds(150000));
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

			bool print_flag = false;
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
							std::cout << " received " << bytes << " but remaining bytes are only " << rem_bytes << "\n";
							if (bytes < 0)
								std::cout << " bytes is negative " << bytes << " but remaining bytes are only " << rem_bytes << "\n";

							util::err("error receiving (more) data", errno);
						}

						ptr += bytes;
						rem_bytes -= bytes;
					}
				}
				print_flag = true;
				// util::err("message size and received message size missmatch");
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
							// std::cout << "Message received: " << p.DebugString() << "\n";
							util::err("register");
							break;
						}

						if (client_msg_req.tostart()) {
							_ptr->context->fibers_txn_data[_ptr->fiber_id]->txn_state = 1;
							_ptr->context->fibers_txn_data[_ptr->fiber_id]->read_values.clear();
							txn_count.fetch_add(1);
							// std::cout << "Message received: " << p.DebugString() << "\n";
							auto txn = (map_txn.find(csock) != map_txn.end()) ? map_txn.find(csock)->second : nullptr;
							if (txn != nullptr) {
								delete txn;
								txn = nullptr;
							}


							// auto txn = txn_db->BeginTransaction(write_options);
							auto txn1 =  new TCoordinator(_ptr->context);
							// auto txn1 = txn_db->BeginTransaction(write_options);
							if (map_txn.find(csock) != map_txn.end()) {
								map_txn.find(csock)->second =  txn1;
								// std::cout << "vrethike ? \n";
							}
							else {
								// std::cout << "kane insert ? gia csock " << csock << "\n";
								map_txn.insert({csock, txn1});
							}
							//             std::cout << "fiber id [ " << std::this_thread::get_id() << "]:" << _ptr->fiber_id << " serves txn " << txn1->GlobalTxn->gettxnId() << "\n";
							//}

					}

					auto _fd = map_fd[csock];
					// std::cout << "sending socket " << _fd << " listening socket " << csock <<  " " << txn_count.load() << "\n";
					// auto txn = map_txn.find(csock)->second;
					auto txn = map_txn[csock];

					auto _stats = nullptr;
					bool res;
					uint64_t ret = execute_txn_req(client_msg_req, txn, _fd, _stats, res, _ptr->fiber_id);
					// std::cout << "execution is done" << _fd << " listening socket " << csock << "\n";
					// _ptr->myfile << "sent bytes to " <<_fd << " " <<  ret << " with message " << res << "\n";
					// _ptr->myfile.flush();

					if (client_msg_req.tocommit()) {
						auto& txn = map_txn.find(csock)->second;
						// std::cout << "Commit csock: " << csock << " txn: " << txn << "\n";
						if (txn == nullptr) {
							util::err("txn is nullptr and we are commiting ..");
						}
						// txn->Commit();
						delete txn;
						txn = nullptr;
					}
					else if (client_msg_req.toabort()) {
						auto& txn = map_txn.find(csock)->second;
						std::cout << "toAbort \n";
						// txn->Rollback();
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
		// boost::this_fiber::sleep_for(std::chrono::microseconds(43));
		boost::this_fiber::yield();
	}

}
}
#endif


#if 0
void coordinator(int* ptr, erpc::Nexus* nexus, AppContext* context) {
	// static std::atomic<int> coordinatorLogId;


	long start_time = get_time();
	std::cout << "start time " << start_time << "\n";
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), *ptr,  sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;

	long end_time = get_time();
	std::cout << "end time " << end_time <<  " " << (end_time - start_time) << "\n";
	std::string martha_uri = kmarthaHostname + ":" + std::to_string(kUDPPort);
	std::string rose_uri = kroseHostname + ":" + std::to_string(kUDPPort);
	int session_num_martha = context->rpc->create_session(martha_uri, FLAGS_cpu_cores - 1 - *ptr);
	int session_num_rose = context->rpc->create_session(rose_uri, (FLAGS_cpu_cores - 1 - *ptr));


	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";

	printf("2pc-eRPC: Coordinator Creating session to %s, Remote RPC ID = %d.\n", martha_uri.c_str(), FLAGS_cpu_cores - 1 - *ptr);

	while (!context->rpc->is_connected(session_num_martha)) context->rpc->run_event_loop_once();

	printf("2pc-eRPC: Coordinator Creating session to %s, Remote RPC ID = %d.\n", rose_uri.c_str(), (FLAGS_cpu_cores - 1 - *ptr));

	while (!context->rpc->is_connected(session_num_rose)) context->rpc->run_event_loop_once();

	connection_t _tmp;
	context->cluster_size = 3;
	context->node_id = 0;
	context->cluster_map[2].session_num = session_num_rose;
	context->cluster_map[0] = _tmp;
	context->cluster_map[1].session_num = session_num_martha;

	printf("2pc-eRPC: All sessions connected.\n");

	sleep(1); // sleep for 5 sec to give time to connections
	auto thread_id = *reinterpret_cast<int*>(ptr);
	std::cout << "thread ready to run ..\n";
	std::cout << "thread id " << thread_id << "\n";
	/*
	   auto _args = reinterpret_cast<threadArgs_boost_fibers*>(args); 
	   */

	std::vector<boost::fibers::fiber> fibers; 
	std::vector<boost_fiber_args*> fiber_args;
	int _s = thread_id;
	for (int i = 0; i < FLAGS_nb_fibers; i++) {
		auto ptr = new boost_fiber_args();
		ptr->context = context;
		// ptr->context = new AppContext();
		/*
		// we do a deep copy
		ptr->context->node_id = context->node_id;
		ptr->context->cluster_map = context->cluster_map;
		ptr->context->cluster_size = context->cluster_size;
		ptr->context->rpc = context->rpc;
		ptr->context->RID = context->RID;
		ptr->context->log = context->log;
		ptr->context->rocksdb_ptr = context->rocksdb_ptr;
		*/
		/*
		   ptr->db = _args->db;
		   ptr->opt_txn_db = _args->opt_txn_db;
		   */
		ptr->index = _s; /* @dimitra nb_worker_threads */
		ptr->fiber_id = (i+1);
		ptr->context->fibers_txn_data[(i+1)] = new AppContext::transaction_info_per_fiber();
		_s += FLAGS_nb_worker_threads;
		fiber_args.push_back(ptr);
		global_fiber_id[(i+1)] = (i+1);
	}

	/*
	   for (int i = 0; i < _args->listening_sockets.size(); i++) {
	   fiber_args[i % nb_fibers]->l_sockets.push_back(_args->listening_sockets[i]);
	   fiber_args[i % nb_fibers]->s_sockets.insert({_args->listening_sockets[i], _args->sending_sockets[_args->listening_sockets[i]]});
	   } 
	   */

	for (int i = 0; i < FLAGS_nb_fibers; i++) {
		fibers.emplace_back(boost::fibers::launch::post, fiber_run_func, fiber_args[i]);
	}

	std::cout << "fibers launched .. good luck!\n";

	for (auto& fiber : fibers)
		fiber.join();

	std::cout << "all fibers joined\n";

}
#endif


#if 0
static void fiber_run_func_participant(boost_fiber_args* _ptr) {
	while (!ctrl_c_pressed) {
		_ptr->context->rpc->run_event_loop_once();
		boost::this_fiber::yield();
		std::this_thread::yield();
	}
}



void participant(int* ptr, erpc::Nexus* nexus, AppContext* context) {

	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), context->RID, sm_handler);

	context->rpc->retry_connect_on_invalid_rpc_id = true;
	context->node_id = 0;
	connection_t _tmp;
	context->cluster_size = 2;
	context->cluster_map[0] = _tmp;
	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";


	std::vector<boost::fibers::fiber> fibers;
	std::vector<boost_fiber_args*> fiber_args;
	for (int i = 0; i < FLAGS_nb_fibers; i++) {
		auto ptr = new boost_fiber_args();
		ptr->context = context;
		ptr->fiber_id = (i+1);
		fiber_args.push_back(ptr);
	}

	for (int i = 0; i < FLAGS_nb_fibers; i++) {
		// for (int i = 0; i < 1; i++) {
		fibers.emplace_back(boost::fibers::launch::post, fiber_run_func_participant, fiber_args[i]);
	}


	std::cout << "fibers launched .. good luck!\n";

	for (auto& fiber : fibers)
		fiber.join();

	delete context->rpc;
	return;
	}
#endif
