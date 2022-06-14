#pragma once
#include <utility>
#include <thread>
#include <signal.h>
#include <tuple>
#include <vector>

#include "common.h"
#include "sample_operations.h"
#include "txn.h"
#include "coordinator.h"
#include "recovered_txn.h"


#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"


void recoverCLOG(AppContext* context);
void recoverFromCLog(CommitLogReader* , AppContext* context);

void coordinatorExecutesTxn(RecoveredOnGoingTxn& txn, AppContext* _c);

void participantExecutesTxn(RecoveredOnGoingTxn& txn, AppContext* _c);

void recoverCLOG(AppContext* context);

void coordinatorExecutesTxn(RecoveredOnGoingTxn& txn, AppContext* _c);

void recoverFromCLog(CommitLogReader* logReader, AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map);
void recoverCLOG(AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& _map);

bool TxnAlreadySeen(int RID, int gid, int tc, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map);
void TxnUpdatePhase(int RID, int gid, int tc, int phase, std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map);
void TxnDelete(int RID, int gid, int tc,std::map<int, std::vector<RecoveredOnGoingTxn>>& recovered_txns_map);

void recoverCLOG(std::shared_ptr<PacketSsl> _packet, AppContext* context, std::map<int, std::vector<RecoveredOnGoingTxn>>& _map);


std::string serialize(std::unordered_map<std::string, std::string> val);
