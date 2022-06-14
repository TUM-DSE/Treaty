#pragma once
#include "local_txn.h"

#include "encrypt_package.h"

// map of the ongoing sub-transactions in this node
// extern std::unordered_map<std::string, LocalTxn> local_txns;
extern CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>* local_txns;

bool transaction_exists(const char* txnId);

LocalTxn* get_transaction(std::string id);

// debugging purposes
void print_local_txns();

extern std::shared_ptr<PacketSsl> txn_cipher;
