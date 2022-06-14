#include "clog.h"

#include "../recovery/recovered_txn.h"

#include <unistd.h>
#include <iostream>
#include <unordered_map>
       #include <sys/time.h>

#undef NDEBUG
#include <cassert>

#define LogEntries	800000
#define MSG_SIZE	1024

std::map<int, std::vector<RecoveredOnGoingTxn>> _maps;

char msg[MSG_SIZE] = "0000000400000005key1value0000000400000005dim1giant\0";

int RIDs[2] = {0, 1};

int coordinatorNodes[2] = {0, 2};


int selectRID(int i) {
	return RIDs[i%2];
}


int selectCoordinator(int i) {
	return coordinatorNodes[i%2];
}


int selectTxnId(int i) {
	static int txn_id = 0;
	return txn_id++;
}


int selectPhase(int i) {
	if (coordinatorNodes[i%2] == 0) {
		if ((i%4) == PRE_PREPARE)
			return (i%4 + 2);
		if ((i%4) == POST_PREPARE)
			return (i%4 + 1);
	}
	if (coordinatorNodes[i%2] == 2) 
		if ((i%4) == POST_PREPARE)
			return PRE_PREPARE;

	return i%4;
}


int selectDesicion(int i) {
	return 1;
}


int selectPairsNumber(int i) {
	return 2;
}


bool logTransaction(CommitLog* log, int& i) {
	// AppendToCLog(int RID, int coordinator, int globalTxn, int phase, int decision = 0, int kvPairsNumber = 0, const char* data = nullptr, size_t data_size = 0);
	int RID = selectRID(i);
	int coordinator = selectCoordinator(i);
	int globalTxn = selectTxnId(i);
	int phase = selectPhase(i);
	int decision = selectDesicion(i);
	int kvPairsNumber = selectPairsNumber(i);
	return log->AppendToCLog(RID, coordinator, globalTxn, phase, decision, kvPairsNumber, msg, 51);
}


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


bool recoverFromCLog(CommitLogReader* logReader) {
	int gid, tc, phase, RID;
	std::unordered_map<std::string, std::string> batch;
	std::unordered_map<int, RecoveredOnGoingTxn> recovered_txns_map;
	bool ret = logReader->RecoverLogEntry(RID, gid, tc, phase, batch);
  uint64_t i = 1;
	std::string s;
	while (ret) {
		// s = ((decode_txn_phase(phase) == "COMMIT") || (decode_txn_phase(phase) == "POST_PREPARE")) ? "" : (" batch: " + serialize(batch));
		// std::cout << "RID : " << RID << " recovered TxnId:" << gid << " with coordinator: " << tc << " phase:" << decode_txn_phase(phase) << s << "]\n";
		_maps[RID].push_back(RecoveredOnGoingTxn(gid, tc, phase, batch));
		ret = logReader->RecoverLogEntry(RID, gid, tc, phase, batch);
    i++;
	}
  std::cout << "recovered: " << i << "\n";

	return true;
}

uint64_t Now()  {
         struct timeval tv;
         gettimeofday(&tv, nullptr);
         return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
       }



int main(int args, char* argv[]) {

	uint8_t __key[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
	uint8_t __iv[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb};


	std::shared_ptr<KeyIV> keyIv(new KeyIV(reinterpret_cast<std::byte*>(__key), reinterpret_cast<std::byte*>(__iv)));
	std::shared_ptr<PacketSsl> _packet = std::make_shared<PacketSsl>(keyIv);

	CommitLog* log = new CommitLog(_packet, "testLog.txt");
	bool success = true;

	for (int i = 0; i < LogEntries; i++)
		success = logTransaction(log, i) && success;

	assert(success);
	delete log;

	
	CommitLogReader* logReader = new CommitLogReader(_packet, "testLog.txt");
	assert(logReader != nullptr);
  auto start = Now();  
	recoverFromCLog(logReader);
  auto end = Now();  

	for (auto txns : _maps) {
		// std::cout << "RID " << txns.first << " should recover " << txns.second.size() << " transactions\n";
		for (auto txn : txns.second) {
			// std::cout << "RID " << txns.first << " txn_id: " << txn.gId << " coordinator: " << txn.tc << " phase: " << decode_txn_phase(txn.phase) << "\n";
		}
	}
  std::cout << "done " << (end - start) << "\n";
	delete logReader;
	
	return 0;
}
