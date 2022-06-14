#pragma once
#include "local_txn.h"

// pointer to a local (per-node) rocksdb instance
rocksdb::TransactionDB* LocalTxn::ptr_db = nullptr;
