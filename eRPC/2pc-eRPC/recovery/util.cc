#include <utility>
#include <thread>
#include <signal.h>
#include <tuple>
#include <vector>

#include "util.h"
#include "common.h"
#include "sample_operations.h"
#include "txn.h"
#include "coordinator.h"
#include "recovered_txn.h"


#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

std::string serialize(std::unordered_map<std::string, std::string> val) {
	std::string s;

	for (auto item : val) {
		s += item.first;
		s += " ";
		s += item.second;
		s += " ";
	}

	return s;
}


void recoverFromCLog(CommitLogReader* logReader, AppContext* context) {
	int rid, gid, tc, phase;
	std::unordered_map<std::string, std::string> batch;
	std::unordered_map<int, RecoveredOnGoingTxn> recovered_txns_map;
	bool ret = logReader->RecoverLogEntry(rid, gid, tc, phase, batch);
	while (ret) {
		fprintf(stdout, "RID: %d, txnID: %d, tc: %d, phase: %s, batch: %s\n", rid, gid, tc, decode_txn_phase(phase).c_str(), serialize(batch).c_str());
		if (phase != COMMIT) {
			// recovered_txns_map.emplace(std::make_pair(gid, std::move(RecoveredOnGoingTxn(gid, tc, phase, batch))));
		}
		batch.clear();
		// ret = logReader->RecoverLogEntry(gid, tc, phase, batch);
		ret = logReader->RecoverLogEntry(rid, gid, tc, phase, batch);
	}
}

bool TxnAlreadySeen(int RID, int gid, int tc, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map) {
	auto _vec = recovered_txns_map[RID];

	for (auto txn : _vec) {
		if ((txn.gId == gid) && (txn.tc == tc))
			return true;
	}
	return false;
}

void TxnUpdatePhase(int RID, int gid, int tc, int phase, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map) {
	auto _vec = recovered_txns_map[RID];

	for (auto txn : _vec) {
		if ((txn.gId == gid) && (txn.tc == tc)) {
			txn.phase = phase;
			break;
		}
	}
}

void TxnDelete(int RID, int gid, int tc,std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map) {
	auto _vec = recovered_txns_map[RID];
	int index = 0;

	for (auto txn : _vec) {
		if ((txn.gId == gid) && (txn.tc == tc)) {
			_vec.erase(_vec.begin() + index);
			break;
		}
		index++;
	}
}

void recoverFromCLog(CommitLogReader* logReader, AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map) {
	int rid, gid, tc, phase;
	std::unordered_map<std::string, std::string> batch;
	bool ret = logReader->RecoverLogEntry(rid, gid, tc, phase, batch);
	while (ret) {
		fprintf(stdout, "RID: %d, txnID: %d, tc: %d, phase: %s, batch: %s\n", rid, gid, tc, decode_txn_phase(phase).c_str(), serialize(batch).c_str());
		if ((phase != COMMIT) && (TxnAlreadySeen(rid, gid, tc, recovered_txns_map))) {
			TxnUpdatePhase(rid, gid, tc, phase, recovered_txns_map);
		}
		else if (phase == COMMIT) {
			TxnDelete(rid, gid, tc, recovered_txns_map);
		}
		else {
			recovered_txns_map[rid].push_back(RecoveredOnGoingTxn(gid, tc, phase, batch));
		}
		batch.clear();
		ret = logReader->RecoverLogEntry(rid, gid, tc, phase, batch);
	}
}

void recoverCLOG(AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& _map) {
	char hostname[64];

	if (gethostname(hostname, 64) < 0)
		fprintf(stderr, "gethostname() failed\n");

	std::string fname = std::string(hostname) + "_commitLog1.txt";

	CommitLogReader* logReader = new CommitLogReader(fname);

	assert(logReader != nullptr);
	recoverFromCLog(logReader, context, _map);

	return;
}


void recoverCLOG(std::shared_ptr<PacketSsl> _packet, AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& _map) {
	char hostname[64];

	if (gethostname(hostname, 64) < 0)
		fprintf(stderr, "gethostname() failed\n");

	std::string fname = std::string(hostname) + "_commitLog1.txt";

	CommitLogReader* logReader = new CommitLogReader(_packet, fname);

	assert(logReader != nullptr);
	recoverFromCLog(logReader, context, _map);

	return;
}

void recoverCLOG(AppContext* context) {
	char hostname[64];

	if (gethostname(hostname, 64) < 0)
		fprintf(stderr, "gethostname() failed\n");

	std::string fname = std::string(hostname) + "_commitLog1.txt";

	CommitLogReader* logReader = new CommitLogReader(fname);

	assert(logReader != nullptr);
	recoverFromCLog(logReader, context);

	return;
}


void coordinatorExecutesTxn(RecoveredOnGoingTxn& txn, AppContext* _c) {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	switch (txn.phase) {
		case POST_PREPARE:
			// update transactions' map and "notify the client about this transaction"
	//		_transactions_map.insert(txn.gId, static_cast<int>(kPrepared));
			// ask participants if this txn is committed 
			break;

		case PRE_PREPARE:
			{
				// at this point the client has invoked prepare before the shutdown
				// the coordinator needs to complete the prepare phase
				// some of the participants may have already prepared the
				// local subtransaction
				// NOTE: we will execute this transactions when participants have replayed their clogs
				TCoordinator _tc(_c, txn.gId);
				// AskRecoveredTxn is synchronous now will block until the participant replies
				_tc.AskRecoveredTxn(txn.gId, txn.tc, txn.txn_batch, false);
			}

			break;
		default:
			assert(false);
			break;
	}

	return;
}



void participantExecutesTxn(RecoveredOnGoingTxn& txn, AppContext* _c) {
	char buf[17];
	buf[16] = '\0';

	// std::cout << __PRETTY_FUNCTION__ << " " << txn.tc << " " << txn.gId << "\n";

	size_t _off = 0;

	_off += snprintf(buf, sizeof(buf), "%08d", txn.tc);
	_off += snprintf(buf + _off, sizeof(buf) - _off, "%08d", txn.gId);

	std::string id(buf, 16);
	std::unique_ptr<LocalTxn> new_txn = std::make_unique<LocalTxn>(std::atoi(id.c_str()));
	bool ack = new_txn.get()->beginLocalTxn();
	assert(ack);

	local_txns->insert(id, std::move(new_txn));

	for (auto op: txn.txn_batch) {

		std::string op_type = std::get<0>(op), key = std::get<1>(op), value = std::get<2>(op);
		// std::cout << "Iterate over operations_set: " << op_type << " " << key << " " << value << "\n";
		if (op_type == "W") {
			//	new_txn.putLocalTxn(key, value);
		}
		else {
			//	new_txn.readLocalTxn(key, &value);
		}
	}

	// bool res = new_txn.prepareLocalTxn(_c->log, _c->RID, txn.tc, txn.gId);

	// TODO: needs to send that this txn has been prepared successfully or failed.

	return;
}




