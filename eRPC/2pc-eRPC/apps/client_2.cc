#include <utility>
#include <thread>
#include <signal.h>
#include <tuple>
#include <vector>

#include "common.h"
#include "sample_operations.h"
#include "txn.h"
#include "coordinator.h"
#include "recovery/recover.h"
#include "recovery/recovered_txn.h"
#include "recovery/util.h"

#include "request_handlers.h"

#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

rocksdb::Options options;
rocksdb::TransactionDBOptions txn_db_options;
rocksdb::TransactionDB* txn_db;

std::string kDBPath = "/tmp/rocksdb_transaction_example_clara"; 

// forward declaration
void coordinator(erpc::Nexus*, AppContext*);
void participant(erpc::Nexus*, AppContext*);

CTSL::HashMap<std::string, erpc::MsgBuffer> Txn::pool_resp; 
CTSL::HashMap<std::string, std::string> Txn::index_table; 
std::atomic<int> Txn::txn_ids;

std::atomic<int> _recovery;

extern std::shared_ptr<PacketSsl> txn_cipher;
/**
 * maps #rpc objects' identifier to recovered
 * transactions.
 * Each thread (which has a unique ID) will iterate
 * over its mapped vector to establish its previous 
 * state.
 */ 
std::map<int, std::vector<RecoveredOnGoingTxn>> recovered_txns_map;

int main(int args, char* argv[]) {
	options.create_if_missing = true;
	rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
	assert(s.ok());

	uint8_t __key[16] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
	uint8_t __iv[12] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb};
	local_txns = new CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>();

	std::shared_ptr<KeyIV> keyIv = std::make_shared<KeyIV>(reinterpret_cast<std::byte*>(__key), reinterpret_cast<std::byte*>(__iv));
	txn_cipher = std::make_shared<PacketSsl>(keyIv);

	LocalTxn::ptr_db = txn_db;

	signal(SIGINT, ctrl_c_handler);
	std::string client_uri = kClientHostname_2 + ":" + std::to_string(kUDPPort);
	erpc::Nexus nexus(client_uri, 0, 0);

	nexus.register_req_func(kReqTxnBegin, req_handler_txnBegin);
	nexus.register_req_func(kReqTxnPrepare, req_handler_txnPrepare);
	nexus.register_req_func(kReqTxnCommit, req_handler_txnCommit);
	nexus.register_req_func(kReqTxnPut, req_handler_txnPut);
	nexus.register_req_func(kReqTxnRead, req_handler_txnRead);
	nexus.register_req_func(kReqTerminate, req_handler_terminate);
	nexus.register_req_func(kReqRecoveredParticipant, req_handler_recoverParticipant);

	AppContext coordinator_context;
	AppContext participant0_context;
	AppContext participant2_context;
	AppContext participant3_context;

	participant0_context.RID = 0;
	coordinator_context.RID = 1;
	participant3_context.RID = 3;
	participant2_context.RID = 2;

	coordinator_context.rocksdb_ptr = txn_db;

	participant0_context.rocksdb_ptr = txn_db;
	participant3_context.rocksdb_ptr = txn_db;

	participant2_context.rocksdb_ptr = txn_db;

	std::cout << "Is this node recovering?\n";
	int reply = 0;
	std::cin >> reply;

	std::atomic_store(&_recovery, reply);


	std::thread txn_coordinator(coordinator, &nexus, &coordinator_context);
	std::thread txn_participant0(participant, &nexus, &participant0_context);
	std::thread txn_participant2(participant, &nexus, &participant2_context);
	std::thread txn_participant3(participant, &nexus, &participant3_context);

	txn_coordinator.join();
	txn_participant0.join();
	txn_participant3.join();
	txn_participant2.join();

	delete local_txns;

	// Cleanup
	delete txn_db;
	DestroyDB(kDBPath, options);

	std::cout << "commits_requested " << commits_requested << "\n";
	std::cout << "commits_served " << commits_served << "\n";
	return 0;
}



void coordinator(erpc::Nexus* nexus, AppContext* context) {

	if (_recovery) {
		recoverCLOG(txn_cipher, context, recovered_txns_map);
		std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << ": CLog is recovered -- participants and coordinator can now read their outstanding transactions ..\n";
		std::atomic_store(&_recovery, 0);
	}


	// logging
	for (auto txns: recovered_txns_map) {
		fprintf(stdout, "*** Thread with RID: %d needs to recover %lu transactions ***\n", txns.first, txns.second.size());
		for (auto each_txn : txns.second) {
			fprintf(stdout, "Thread with RID: %d \t txn_id: %d \t tc: %d \t phase: %d\n", txns.first, each_txn.gId, each_txn.tc, each_txn.phase);
		}
	}

	auto _vec = recovered_txns_map[context->RID];

	// establish connection
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), 1, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;

	// create a session to each remote thread
	std::string uri = kServerHostname + ":" + std::to_string(kUDPPort);
	int session_num = context->rpc->create_session(uri, 1);
	printf("2pc-eRPC: Creating session to %s, Remote RPC ID = 1.\n", uri.c_str());

	while (!context->rpc->is_connected(session_num)) context->rpc->run_event_loop_once();

	printf("2pc-eRPC: Client connected to all. Sending reqs.\n");

	connection_t _tmp;
	context->cluster_size = 1;
	context->node_id = 2;
	context->cluster_map[0] = _tmp;
	context->cluster_map[0].session_num = session_num;

	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";

	printf("2pc-eRPC: All sessions connected.\n");
	// TODO: if participant ready (Participant Recovered successfully) execute coordinatorExecutesTxn()

	std::cin >> participant_recovered;
	while (!participant_recovered);

	for (auto txn: _vec) {
		coordinatorExecutesTxn(txn, context);
	}

	std::cout << "[Sent that we are done]\n";
	TCoordinator tc11(context);
	tc11.AskRecoveredTxn(111, 0, std::vector<std::tuple<std::string, std::string, std::string>> {}, true);

	context->rpc->run_event_loop(200);	
	context->rpc->run_event_loop(200);	
	context->rpc->run_event_loop(200);	
	context->rpc->run_event_loop(200);	
	context->rpc->run_event_loop(200);	
	// established_state  = 1;


	/* Operate Normally */
	std::cout << __PRETTY_FUNCTION__ << ": [thread-" << std::this_thread::get_id() << "] now operates normally ..\n";

	//context->log = context->create_commit_log("tcLog" + std::to_string(context->node_id) + "0.txt");
	int request_type = 0;
	usleep(10000);
	bool ok;
	while (!ctrl_c_pressed) {

		usleep(10000);

		TCoordinator tc(context);
		tc.Put("key11", "value");
		context->rpc->run_event_loop_once();


		tc.Put("dimitra1", "giantsidi");
		context->rpc->run_event_loop_once();	
		tc.Put("foivos1", "tsimp");
		context->rpc->run_event_loop_once();	
		ok = tc.Prepare();
		context->rpc->run_event_loop_once();	
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
		context->rpc->run_event_loop(200);	

		TCoordinator tc2(context);
		tc2.Put("key1", "ante gamisou");
		tc2.Put("dimitra", "foivakos");
		tc2.Read("key1");
		tc2.Read("dimitra");
		ok = tc2.Prepare();
		if (ok && !ctrl_c_pressed)
			tc2.CommitTransaction();
		context->rpc->run_event_loop(200);
		request_type++;
		if (request_type == 200)
			ctrl_c_pressed = 1;
	}
	delete context->rpc;
}


erpc::MsgBuffer req__;
erpc::MsgBuffer resp__;
int ready_to_go = 0;

static void cont_func_ReadyToGo(void*, void*) {
	std::cout << __PRETTY_FUNCTION__ << " " << resp__.buf << "\n";
	ready_to_go = 1;
}

void participant(erpc::Nexus* nexus, AppContext* context) {

	while (_recovery.load()) {}

	std::cout << " ------------------- [Phase 1 - " << std::this_thread::get_id()<< "] RECOVER: recovered on-going transactions are now in the map\n";

	auto _recovered_txns =  recovered_txns_map[context->RID];

	for (auto txn : _recovered_txns) {
		participantExecutesTxn(txn, context);
	}
	std::cout << " ------------------- [Phase 2 - " << std::this_thread::get_id() << "] RECOVERY DONE: participant " << " with RID: " << context->RID << " established its previous state" << "\n";


	/* Free to go on! */
	static std::atomic<int> participantLogId;
	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), context->RID, sm_handler);
	context->rpc->retry_connect_on_invalid_rpc_id = true;
	context->node_id = 2;
	connection_t _tmp;
	context->cluster_size = 1;
	context->cluster_map[1] = _tmp;


	// establish connection with the mapped coordinator
	std::string uri = kServerHostname + ":" + std::to_string(kUDPPort);
	int session_num = context->rpc->create_session(uri, 0);
	while (!context->rpc->is_connected(session_num)) context->rpc->run_event_loop_once();


	req__ = context->rpc->alloc_msg_buffer_or_die(kMsgSize);
	sprintf(reinterpret_cast<char *>(req__.buf), "successfull_recovery");
	resp__ = context->rpc->alloc_msg_buffer_or_die(kMsgSize);

	std::cout << " ENQUEUE_REQUEST THAT WE RECOVERED\n";
	context->rpc->enqueue_request(session_num, kReqRecoveredParticipant, &req__, &resp__, cont_func_ReadyToGo, nullptr);
	std::cout << " ENQUEUE_REQUEST THAT WE RECOVERED\n";
	context->rpc->run_event_loop(2000);

	while (!ready_to_go) {}


	// while (!established_state) {}
	std::cout << __PRETTY_FUNCTION__ << ": [thread-" << std::this_thread::get_id() << "] now operates normally ..\n";

	// TODO: sent all messages to the corresponding coordinator

	// context->log = context->create_commit_log("pLog" + std::to_string(context->node_id) + std::to_string(std::atomic_fetch_add(&participantLogId, 1)) + ".txt");
	while (!ctrl_c_pressed) {
		context->rpc->run_event_loop(10000);
	}

	delete context->rpc;
	return;
}

