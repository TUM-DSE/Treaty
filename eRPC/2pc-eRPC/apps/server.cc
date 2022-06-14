#include <thread>

#include "common.h"
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

rocksdb::Options options;
rocksdb::TransactionDBOptions txn_db_options;
rocksdb::TransactionDB* txn_db;

std::string kDBPath = "/tmp/rocksdb_transaction_example_server";

// forward declaration
void coordinator(erpc::Nexus*, AppContext*);
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
	std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);

	
	erpc::Nexus nexus(server_uri, 0, 0);

	nexus.register_req_func(kReqTxnBegin, req_handler_txnBegin);
	nexus.register_req_func(kReqTxnPrepare, req_handler_txnPrepare);
	nexus.register_req_func(kReqTxnCommit, req_handler_txnCommit);
	nexus.register_req_func(kReqTxnPut, req_handler_txnPut);
	nexus.register_req_func(kReqTxnRead, req_handler_txnRead);
	nexus.register_req_func(kReqTerminate, req_handler_terminate);
	nexus.register_req_func(kReqRecoveredParticipant, req_handler_recoverParticipant);
	nexus.register_req_func(kReqRecoveredCoordinator, req_handler_recoverCoordinator);


	AppContext coordinator_context;
	AppContext participant1_context;
	AppContext participant2_context;
	AppContext participant3_context;

	participant1_context.RID = 1;
	participant2_context.RID = 2;
	participant3_context.RID = 3;

	coordinator_context.rocksdb_ptr = txn_db;
	participant1_context.rocksdb_ptr = txn_db;

	participant2_context.rocksdb_ptr = txn_db;
	participant3_context.rocksdb_ptr = txn_db;

	std::thread txn_coordinator(coordinator, &nexus, &coordinator_context);
	std::thread txn_participant1(participant, &nexus, &participant1_context);

	std::thread txn_participant2(participant, &nexus, &participant2_context);
	std::thread txn_participant3(participant, &nexus, &participant3_context);

	txn_coordinator.join();

	txn_participant1.join();

	txn_participant2.join();
	txn_participant3.join();

	local_txns->clear();
	delete local_txns;

	// Cleanup
	delete txn_db;
	DestroyDB(kDBPath, options);

	_args.to_string();
	std::cout << "commits_requested: " << commits_requested << "\n";
	std::cout << "commits_served: " << commits_served << "\n";

	return 0;
}


void coordinator(erpc::Nexus* nexus, AppContext* context) {
	// static std::atomic<int> coordinatorLogId;
	
	long start_time = get_time();
	std::cout << "start time " << start_time << "\n";
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), 0, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;

	long end_time = get_time();
	std::cout << "end time " << end_time <<  " " << (end_time - start_time) << "\n";
	std::string amy_uri = kClientHostname_1 + ":" + std::to_string(kUDPPort);
	int session_num_1 = context->rpc->create_session(amy_uri, 0);

	std::string clara_uri = kClientHostname_2 + ":" + std::to_string(kUDPPort);
	int session_num_2 = context->rpc->create_session(clara_uri, 0);

	std::cout << __PRETTY_FUNCTION__ << std::this_thread::get_id() << "\n";

	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 0.\n", amy_uri.c_str());
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 0.\n", clara_uri.c_str());

	while (!context->rpc->is_connected(session_num_1)) context->rpc->run_event_loop_once();
	while (!context->rpc->is_connected(session_num_2)) context->rpc->run_event_loop_once();

	connection_t _tmp;
	context->cluster_size = 3;
	context->node_id = 0;
	context->cluster_map[1] = _tmp;
	context->cluster_map[0] = _tmp;
	context->cluster_map[2] = _tmp;
	context->cluster_map[1].session_num = session_num_1;
	context->cluster_map[2].session_num = session_num_2;

	printf("2pc-eRPC: All sessions connected.\n");

	// while (!established_state) { std::cout << " HERE\n"; }


	printf("[--- Previous State Established ---].\n");
	int txns_number = 0;
	bool ok;
	while (!ctrl_c_pressed) {

		TCoordinator tc(context);
		tc.Put(msg::randomKey(4), "value");
		tc.Put(msg::randomKey(10), "giantsidi");
		tc.Put(msg::randomKey(1), "tsimp");
		tc.Put(msg::randomKey(5), "tsimp");
		ok = tc.Prepare();

		if (ok && !ctrl_c_pressed)
			tc.CommitTransaction();
		context->rpc->run_event_loop_once();


		TCoordinator tc1(context);
		tc1.Read("key1");
		tc1.Read("dimitra");
		tc1.Read("foivos");
		ok = tc1.Prepare();
		if (ok && !ctrl_c_pressed)
			tc1.CommitTransaction();
		context->rpc->run_event_loop_once();

		TCoordinator tc2(context);
		tc2.Put("key1", "ante gamisou");
		tc2.Put("dimitra", "foivakos");
		tc2.Read("key1");
		tc2.Read("dimitra");
		ok = tc2.Prepare();
		if (ok && !ctrl_c_pressed)
			tc2.CommitTransaction();
		context->rpc->run_event_loop_once();

		txns_number++;
		if (txns_number >= transactions_num)
			ctrl_c_pressed = true;
	}

	TCoordinator tc(context);
	tc.Terminate(session_num_1);
	tc.Terminate(session_num_2);

	context->rpc->run_event_loop(1000);

	delete context->rpc;
	return;
}



void participant(erpc::Nexus* nexus, AppContext* context) {
	// static std::atomic<int> participantLogId;
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), context->RID, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;
	connection_t _tmp;
	context->cluster_size = 3;
	context->node_id = 0;
	context->cluster_map[1] = _tmp;
	context->cluster_map[2] = _tmp;
	context->cluster_map[0] = _tmp;
	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";

	// while (!established_state) {}
	printf("[--- Previous State Established ---].\n");

	while (!ctrl_c_pressed) {
		context->rpc->run_event_loop(1000);
	}

	delete context->rpc;
	return;
}
