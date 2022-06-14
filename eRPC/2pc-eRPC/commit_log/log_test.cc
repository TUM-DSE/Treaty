#include "clog.h"

#include <unistd.h>
#include <iostream>
#include <unordered_map>

#undef NDEBUG
#include <cassert>

#define LogEntries	4
#define MSG_SIZE	1024

char msg[MSG_SIZE] = "0000000400000005key1value0000000400000005dim1giant\0";


class RecoveredOnGoingTxn {
	public:
		using key = std::string;
		using value = std::string;
		using _write_batch = std::unordered_map<key, value>;

		RecoveredOnGoingTxn(const RecoveredOnGoingTxn& other) = delete;
		void operator=(const RecoveredOnGoingTxn& other) = delete;

		RecoveredOnGoingTxn() = delete;

		RecoveredOnGoingTxn(int globalId, int Coordinator, int ph, _write_batch batch = {}) : gId(globalId), tc(Coordinator), phase(ph) {
			assert(phase != COMMIT);
			txn_batch = std::move(batch);
		};

		~RecoveredOnGoingTxn() {};

		bool emptyBatch() { 
			return (txn_batch.size() == 0);
		}

		RecoveredOnGoingTxn(RecoveredOnGoingTxn&& other) {
#ifdef DEBUG_EXTRA
			std::cout << __PRETTY_FUNCTION__ << "\n";
#endif
			gId = std::move(other.gId);
			tc = std::move(other.tc);
			phase = std::move(other.phase);
			if (!emptyBatch())
				txn_batch = std::move(other.txn_batch);
		}

		void operator=(RecoveredOnGoingTxn&& other) {
			gId = std::move(other.gId);
			tc = std::move(other.tc);
			phase = std::move(other.phase);
			if (!emptyBatch())
				txn_batch = std::move(other.txn_batch);
		}


	private:
		int gId, tc, phase;
		_write_batch txn_batch;
		// txn_handle;

};

bool logTransaction(CommitLog* log, int& i) {
	return log->AppendToCLog(601, i*11111, i, i%4, 1, 2, msg, 51);
}

std::string serialize(std::unordered_map<std::string, std::string> val) {
	std::string s;

	for (auto item : val) {
		s += item.first;
		s += " ";
		s += item.second;
#ifdef DEBUG_EXTRA
		std::cout << item.first << " " << item.second << "\n";
#endif
		s += " ";
	}

	return s;
}

bool recoverFromCLog(CommitLogReader* logReader) {
	int gid, tc, phase;
	std::unordered_map<std::string, std::string> batch;
	std::unordered_map<int, RecoveredOnGoingTxn> recovered_txns_map;
	bool ret = logReader->RecoverLogEntry(gid, tc, phase, batch);
	while (ret) {
		std::cout << "Recovered Txn [TxnId:" << gid << " " << tc << " phase:" << decode_txn_phase(phase) << " batch:" << serialize(batch) << "]\n";
		if (phase != COMMIT) {
			recovered_txns_map.emplace(std::make_pair(gid, std::move(RecoveredOnGoingTxn(gid, tc, phase, batch))));
		}
		batch.clear();
		ret = logReader->RecoverLogEntry(gid, tc, phase, batch);
	}

	return true;
}

int main(int args, char* argv[]) {
	CommitLog* log = new CommitLog("testLog.txt");
	bool success = true;

	for (int i = 0; i < LogEntries; i++)
		success = logTransaction(log, i) && success;

	assert(success);
	delete log;

	CommitLogReader* logReader = new CommitLogReader("testLog.txt");
	assert(logReader != nullptr);

	// std::cout << logReader->RecoverLog() << "\n";
	// logReader->RecoverLog();
	recoverFromCLog(logReader);

	delete logReader;
	return 0;
}
