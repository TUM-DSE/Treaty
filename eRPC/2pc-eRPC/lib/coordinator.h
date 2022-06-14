#pragma once
#include <stdio.h>
#include <signal.h>
#include <unordered_map>
#include <map>
#include <tuple>
#include <vector>

#include "rpc.h"
#include "txn.h"
#include "debug.h"

#include "txn_phases.h"

#undef NDEBUG 	// this enables assertions but by default (in the Makefile) it is 'defined' to make rocksdb compile
#include <cassert>

class TCoordinator {
	public:
		Txn* GlobalTxn = nullptr;

		AppContext *_c;

		TCoordinator(AppContext* c): _c(c) {
			GlobalTxn = new Txn(c);
			assert(c != nullptr);
		}

		TCoordinator(AppContext* c, int id): _c(c) {
			GlobalTxn = new Txn(c, id);
			assert(c != nullptr);
		}

		~TCoordinator() {
			if (GlobalTxn != nullptr)
				delete GlobalTxn;
		}


		int findParticipant(std::string& key) {
#if TPCC_TEST
			return 0;
#endif
			auto id = std::hash<std::string>{}(key)%(_c->cluster_size);
			return id;
		}

		void AskRecoveredTxn(int globaltxnid, int txncoordinator, std::vector<std::tuple<std::string, std::string, std::string>> op_set, bool normal) {
			// @dimitra: not really used
			if (!normal)
			assert(txncoordinator == _c->node_id);
			GlobalTxn->AskRecoveredTxn(globaltxnid, -1, txncoordinator, op_set, normal);
		}

		void Terminate(int session) {
			// terminates the session (nothing related to rocksdb)
			GlobalTxn->Terminate(session);
		}

		bool BeginTransaction() { 
			return true; 
		}

		bool Prepare(int fiber_id) {
			return GlobalTxn->Prepare(fiber_id);
		}

		bool CommitTransaction(int fiber_id) {
			return GlobalTxn->Commit(fiber_id);
		}

		bool Put(std::string key, std::string value, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->Put(key, value, NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		bool Put(std::string& key, std::string& value, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->Put(key, value, NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		/* @dimitra TODO: maybe add an emum/class with possible retvalues
		 * Read()/ReadForUpdate() return
		 * 1 : if value is in value
		 * 0 : if not found
		 * -1 : if error happened
		 *  2 : if value is served by another node
		 */
		int Read(std::string& key, std::string& value, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->Read(key, value, NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		int Read(std::string key, std::string& value, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->Read(key, value,  NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		int ReadForUpdate(std::string key, std::string& value, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->ReadForUpdate(key, value,  NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		bool Delete(std::string key, int fiber_id) {
			int NodeId = findParticipant(key);
			return GlobalTxn->Delete(key, NodeId, _c->getSessionOfNode(NodeId), fiber_id);
		}

		void wait_for_all() {
			GlobalTxn->wait_for_all();
		}

		void Rollback(int fiber_id) {
			GlobalTxn->Rollback(fiber_id);
		}
};
