#include <utility>
#include <thread>
#include <signal.h>

#include "common.h"
#include "sample_operations.h"
#include "txn.h"
#include "coordinator.h"

#include "request_handlers.h"

#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include "args_parser/args_parser.h"

rocksdb::Options options;
rocksdb::TransactionDBOptions txn_db_options;
rocksdb::TransactionDB* txn_db;

std::string kDBPath = "/tmp/rocksdb_transaction_example_amy"; 

// forward declaration
void coordinator(erpc::Nexus*, AppContext*);
void coordinator_test(erpc::Nexus*, AppContext*);
void participant(erpc::Nexus*, AppContext*);

CTSL::HashMap<std::string, erpc::MsgBuffer> Txn::pool_resp; 
CTSL::HashMap<std::string, std::string> Txn::index_table; 
std::atomic<int> Txn::txn_ids;

extern std::shared_ptr<PacketSsl> txn_cipher;

static int coordinators_num = 0;
static int transactions_num = 0;

int main(int argc, char* argv[]) {
	args_parser::ArgumentParser _args("speicherdb");
	_args.parse_input(argc, argv);

	coordinators_num = _args.get_num_of_coordinators();
	transactions_num = _args.get_num_of_txns();

	uint8_t __key[16] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
	uint8_t __iv[12] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb};
	local_txns = new CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>();

	std::shared_ptr<KeyIV> keyIv = std::make_shared<KeyIV>(reinterpret_cast<std::byte*>(__key), reinterpret_cast<std::byte*>(__iv));
	txn_cipher = std::make_shared<PacketSsl>(keyIv);

	options.create_if_missing = true;
	rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
	assert(s.ok());
	LocalTxn::ptr_db = txn_db;

	signal(SIGINT, ctrl_c_handler);
	std::string client_uri = kClientHostname_1 + ":" + std::to_string(kUDPPort);


	erpc::Nexus nexus(client_uri, 0, 0);

	nexus.register_req_func(kReqTxnBegin, req_handler_txnBegin);
	nexus.register_req_func(kReqTxnPrepare, req_handler_txnPrepare);
	nexus.register_req_func(kReqTxnCommit, req_handler_txnCommit);
	nexus.register_req_func(kReqTxnPut, req_handler_txnPut);
	nexus.register_req_func(kReqTxnRead, req_handler_txnRead);
	nexus.register_req_func(kReqTerminate, req_handler_terminate);
	nexus.register_req_func(kReqRecoveredParticipant, req_handler_recoverParticipant);


	AppContext coordinator1_context;
	AppContext coordinator2_context;
	AppContext participant_context;

	coordinator1_context.rocksdb_ptr = txn_db;
	coordinator2_context.rocksdb_ptr = txn_db;
	participant_context.rocksdb_ptr = txn_db;

	// usleep(10000);

	std::thread txn_coordinator1(coordinator, &nexus, &coordinator1_context);
	std::thread txn_coordinator2(coordinator_test, &nexus, &coordinator2_context);
	std::thread txn_participant(participant, &nexus, &participant_context);

	txn_coordinator1.join();
	txn_coordinator2.join();
	txn_participant.join();

	delete local_txns;

	// Cleanup
	delete txn_db;
	DestroyDB(kDBPath, options);

	std::cout << "commits_requested " << commits_requested << "\n";
	std::cout << "commits_served " << commits_served << "\n";
	return 1;
}

void coordinator(erpc::Nexus* nexus, AppContext* context) {
	long start_time = get_time();
	std::cout << "start time " << start_time << "\n";
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), 1, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;
	long end_time = get_time();
	std::cout << "end time " << end_time <<  " " << (end_time - start_time) << "\n";

	// Create a session to each server
	std::string uri = kServerHostname + ":" + std::to_string(kUDPPort);
	int session_num = context->rpc->create_session(uri, 2);
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 2.\n", uri.c_str());

	uri = kClientHostname_2 + ":" + std::to_string(kUDPPort);
	int session_num_2 = context->rpc->create_session(uri, 2);
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 2.\n", uri.c_str());

	while (!context->rpc->is_connected(session_num)) context->rpc->run_event_loop_once();
	while (!context->rpc->is_connected(session_num_2)) context->rpc->run_event_loop_once();

	printf("2pc-eRPC: Client connected to all. Sending reqs.\n");

	connection_t _tmp;
	context->cluster_size = 3;
	context->node_id = 1;
	context->cluster_map[0] = _tmp;
	context->cluster_map[0].session_num = session_num;
	context->cluster_map[2] = _tmp;
	context->cluster_map[2].session_num = session_num_2;
	context->cluster_map[1] = _tmp; // this is me!
	std::cout << __PRETTY_FUNCTION__ << "  DASKDJASKDJSKDJSKDJSADSAK " << std::this_thread::get_id() << "\n";
	// context->log = context->create_commit_log("tcLog" + std::to_string(context->node_id) + "0.txt");
	printf("2pc-eRPC: All sessions connected.\n");
	int txns_number = 0;
	bool ok;
	while (!ctrl_c_pressed && context->rpc->is_connected(session_num)  && context->rpc->is_connected(session_num_2)) {

		TCoordinator tc(context);
		tc.Put("key2", "value");
		tc.Put(msg::randomKey(2), "value");
		tc.Put(msg::randomKey(4), "giantsidi");
		tc.Put(msg::randomKey(16), "tsimp");

		ok = tc.Prepare();
		if (ok && !ctrl_c_pressed)
			tc.CommitTransaction();
		context->rpc->run_event_loop_once();	

		TCoordinator tc2(context);
		tc2.Put(msg::randomKey(5), "ante gamisou");
		tc2.Put(msg::randomKey(9), "foivakos");
		tc2.Read("key2");
		ok = tc2.Prepare();
		if (ok && !ctrl_c_pressed)
			tc2.CommitTransaction();
		context->rpc->run_event_loop_once();

		/**
		 * TCoordinator tc3(context);
		 * tc3.AskRecoveredTxn(2, 0);
		 */

		txns_number++;
		if (txns_number == 2000)
			ctrl_c_pressed = 0;
	}
	delete context->rpc;
}

void participant(erpc::Nexus* nexus, AppContext* context) {
	static std::atomic<int> participantLogId;
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), 0, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;
	context->node_id = 1;
	connection_t _tmp;
	context->cluster_size = 3;
	context->cluster_map[0] = _tmp;
	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";
	// context->log = context->create_commit_log("pLog" + std::to_string(context->node_id) + std::to_string(std::atomic_fetch_add(&participantLogId, 1)) + ".txt");

	while (!ctrl_c_pressed) {
		context->rpc->run_event_loop(1000);
	}

	std::cout << __PRETTY_FUNCTION__ << " AYTO TERMATIZEI :) " << std::this_thread::get_id() << "\n";
	delete context->rpc;
	return;
}

void coordinator_test(erpc::Nexus* nexus, AppContext* context) {
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), 3, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;

	// Create a session to each server
	std::string uri = kServerHostname + ":" + std::to_string(kUDPPort);
	int session_num = context->rpc->create_session(uri, 3);
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 3.\n", uri.c_str());

	uri = kClientHostname_2 + ":" + std::to_string(kUDPPort);
	int session_num_2 = context->rpc->create_session(uri, 3);
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 3.\n", uri.c_str());

	while (!context->rpc->is_connected(session_num)) context->rpc->run_event_loop_once();

	while (!context->rpc->is_connected(session_num_2)) context->rpc->run_event_loop_once();

	printf("2pc-eRPC: Client connected to all. Sending reqs.\n");

	connection_t _tmp;
	context->cluster_size = 3;
	context->node_id = 1;
	context->cluster_map[0] = _tmp;
	context->cluster_map[0].session_num = session_num;
	context->cluster_map[2] = _tmp;
	context->cluster_map[2].session_num = session_num_2;
	context->cluster_map[1] = _tmp; // this is me!
	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << " " << session_num << "\n";
	// context->log = context->create_commit_log("tcLog" + std::to_string(context->node_id) + "1.txt");

	printf("2pc-eRPC: All sessions connected.\n");
	int txns_number = 0;
	bool ok;
	while (!ctrl_c_pressed && context->rpc->is_connected(session_num) && context->rpc->is_connected(session_num_2)) {

		TCoordinator tc(context);
		tc.Put(msg::randomKey(4), "value");
		tc.Put(msg::randomKey(16), "giantsidi");
		tc.Put("dimitroula222222", "giantsidi");
		tc.Put(msg::randomKey(28), "tsimp");
		tc.Put(msg::randomKey(1), "tsimp");
		ok = tc.Prepare();
		if (ok && !ctrl_c_pressed)
			tc.CommitTransaction();
		context->rpc->run_event_loop_once();	

		TCoordinator tc1(context);
		tc1.Read("key222222");
		tc1.Read("dimitroula222222");
		tc1.Read("foivakos222222222");
		ok = tc1.Prepare();
		if (ok && !ctrl_c_pressed)
			tc1.CommitTransaction();
		context->rpc->run_event_loop_once();	

		TCoordinator tc2(context);
		tc2.Put(msg::randomKey(8), "ante gamisou");
		tc2.Put("dimitra2", "foivakos");
		tc2.Read("key12");
		tc2.Read("dimitra2");
		ok = tc2.Prepare();
		if (ok && !ctrl_c_pressed)
			tc2.CommitTransaction();
		context->rpc->run_event_loop_once();

		txns_number++;
		if (txns_number >= transactions_num)
			ctrl_c_pressed = 0;
	}
	delete context->rpc;
}
