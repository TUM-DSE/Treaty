/**
 * class LocalTxn represents a local sub-transaction that is part of a distributed transaction.
 */

#pragma once

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include "debug.h"
#include "transactions_info.h"
#include "commit_log/clog.h"
#include "include/sample_operations.h"
class LocalTxn {
  public:
    // LocalTxn() = delete;

    LocalTxn() {
	    write_options.sync = true;
      write_options.disableWAL = 0;
    }

    LocalTxn(int id) : txnId(id) {
    write_options.sync = true;
      write_options.disableWAL = 0;
    };

    ~LocalTxn(){
      // std::cout << __PRETTY_FUNCTION__ << "\n";
      // serialize_batch();
      if (txn_handle != nullptr)
        delete txn_handle;
    };

    // TODO: check what it is going on here
    // delete copy constructor (and assignement operator)
    //	LocalTxn(LocalTxn const & txn) = delete;
    //	LocalTxn& operator = (LocalTxn const& other) = delete;

    LocalTxn(LocalTxn const & txn) = delete;
    LocalTxn& operator = (LocalTxn const& other) = delete;

    /*
       LocalTxn(const LocalTxn& txn) {
    // std::cout << __PRETTY_FUNCTION__ << "\n";
    this->txn_handle = txn.txn_handle;
    txn.txn_handle = nullptr;
    this->txnId = txn.txnId;
    }

    LocalTxn& operator = (LocalTxn txn) {
    // std::cout << __PRETTY_FUNCTION__ << "\n";
    this->txn_handle = txn.txn_handle;
    txn.txn_handle = nullptr;
    this->txnId = txn.txnId;
    return *this;
    }*/

    LocalTxn(LocalTxn&& txn) {
      // std::cout << __PRETTY_FUNCTION__ << "\n";
      this->txn_handle = txn.txn_handle;
      this->txnId = txn.txnId;
      txn.txn_handle = nullptr;
      this->_write_batch = std::move(txn._write_batch);
      this->txnState = txn.txnState;
      this->tc = txn.tc;
      this->tcId = txn.tcId;
      this->read_options = txn.read_options;
      this->write_options = txn.write_options;
    }

    LocalTxn& operator = (LocalTxn&& txn) {
      // std::cout << __PRETTY_FUNCTION__ << "\n";
      this->txn_handle = txn.txn_handle;
      txn.txn_handle = nullptr;
      this->txnId = txn.txnId;
      this->_write_batch = std::move(txn._write_batch);
      this->txnState = txn.txnState;
      this->tc = txn.tc;
      this->tcId = txn.tcId;
      this->read_options = txn.read_options;
      this->write_options = txn.write_options;

      return *this;
    }

    // delete move constructor (and move operator)
    // LocalTxn(LocalTxn&& txn) = delete;
    // LocalTxn& operator = (LocalTxn&& other) = delete;

    int state() {
      return txnState;
    };

    bool beginLocalTxn() {
      /**
       * Create a new transaction; currently we only
       * consider pessimistic transactions because that
       * way we have already a two-phase locking implemented.
       */
      // std::cout << __PRETTY_FUNCTION__ << "\n";
      txn_handle = ptr_db->BeginTransaction(write_options);
      assert(txn_handle);
      bool ret = (txn_handle) ? true : false;
      txnState = kInProgress;
      return ret;
    };

    bool commitLocalTxn(CommitLog* log, int RID, int coordinatorId, int globalTxnId) {
      assert(txnState == kPrepared);
      // rocksdb::Status s = txn_handle->Commit(log, coordinatorId, globalTxnId);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->Commit();
#else
      rocksdb::Status s;
#endif
      // std::cout << __PRETTY_FUNCTION__ << " " << txn_handle->GetID() << "\n";

      bool ret = (s.ok()) ? true : false;
      txnState = (ret) ? kCommitted : kAborted;
      return ret;
    }

    bool RollbackLocalTxn() {
      assert(txnState == kPrepared);
      // rocksdb::Status s = txn_handle->Commit(log, coordinatorId, globalTxnId);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->Rollback();
#else
      rocksdb::Status s;
#endif
      // std::cout << __PRETTY_FUNCTION__ << " " << txn_handle->GetID() << "\n";
      bool ret = (s.ok()) ? true : false;
      txnState = (ret) ? kCommitted : kAborted;
      return ret;
    }

    bool prepareLocalTxn(CommitLog* log, int RID, int coordinatorId, int globalTxnId) {
#if LOG_ON
      std::string s = serialize_batch();
      rocksdb::Status st = txn_handle->_Prepare(log, RID, coordinatorId, globalTxnId, _write_batch.size(), const_cast<char*>(s.c_str()), s.size());
#endif


#ifdef NO_STORAGE
      rocksdb::Status st = txn_handle->Prepare();
#else
      rocksdb::Status st;
#endif

      // txnState = kPrepared;
      // bool ret = (st.ok()) ? true : false; //TODO
      bool ret = true;
      txnState = (ret) ? kPrepared : kAborted;
      // LOG_DEBUG_MSG(__FILE__, __LINE__, __PRETTY_FUNCTION__, st.ToString());
      return ret;
    }

    bool putLocalTxn(std::string& key, std::string& value) {
      assert(txn_handle);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->Put(key, value);
#else
      rocksdb::Status s;
#endif
      _write_batch.insert(std::make_pair(key, value));
      bool ret = false;
      if (s.ok()) {
        ret = true;
        // fprintf(stdout, "Transaction::Put %s -- %s\n", s.ToString().c_str(), key.c_str());
      }

      else {
        fprintf(stdout, "Transaction::Put %s -- %s\n", s.ToString().c_str(), key.c_str());
      }

      // LOG_DEBUG_MSG(__FILE__, __LINE__, __PRETTY_FUNCTION__, s.ToString());
      return ret;
    }

    bool deleteLocalTxn(std::string& key) {
      assert(txn_handle);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->Delete(key);
#else
      rocksdb::Status s;
#endif
      _write_batch.insert(std::make_pair(key, " "));
      bool ret = false;
      if (s.ok()) {
        ret = true;
      }
      else {
        fprintf(stdout, "Transaction::Delete %s -- %s\n", s.ToString().c_str(), key.c_str());
      }

      return ret;
    }

    int readLocalTxn(std::string& key, std::string* value) {
      assert(txn_handle);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->Get(read_options, key, value);
#else
      rocksdb::Status s;
      *value = msg::getVal(1000);
#endif
      // rocksdb::Status s;
      // 	fprintf(stdout, "Transaction::Read %s -- %s -- %lu\n", s.ToString().c_str(), key.c_str(), txn_handle->GetID());
      int ret;
      if (s.ok()) {
        ret = 1;
        //	std::cout << __PRETTY_FUNCTION__ << " " << (*value) << "\n";
      }
      else if (s.IsNotFound()) {
        fprintf(stdout, "Transaction::Read %s -- %s\n", s.ToString().c_str(), key.c_str());
        ret = 0;
      }
      else {
        ret = -1;
        fprintf(stdout, "Transaction::Read %s -- %s\n", s.ToString().c_str(), key.c_str());
      }
      // LOG_DEBUG_MSG(__FILE__, __LINE__, __PRETTY_FUNCTION__, s.ToString());
      return ret;
    }

    int readForUpdateLocalTxn(std::string& key, std::string* value) {
      assert(txn_handle);
#ifdef NO_STORAGE
      rocksdb::Status s = txn_handle->GetForUpdate(read_options, key, value);
#else
      rocksdb::Status s;
#endif
      // fprintf(stdout, "Transaction::ReadForUpdate %s -- %s -- %lu\n", s.ToString().c_str(), key.c_str(), txn_handle->GetID());
      int ret;
      if (s.ok()) {
        ret = 1;
        //fprintf(stdout, "Transaction::ReadForUpdate %s -- %s -- %lu\n", s.ToString().c_str(), key.c_str(), value->size());
        //	fprintf(stdout, "Transaction::ReadForUpdate %s -- %s\n", s.ToString().c_str(), key.c_str());
      }
      else if (s.IsNotFound()) {
        fprintf(stdout, "Transaction::ReadForUpdate %s -- %s\n", s.ToString().c_str(), key.c_str());
        ret = 0;
      }
      else {
        ret = -1;
        fprintf(stdout, "Transaction::ReadForUpdate %s -- %s\n", s.ToString().c_str(), key.c_str());
      }
      // LOG_DEBUG_MSG(__FILE__, __LINE__, __PRETTY_FUNCTION__, s.ToString());
      return ret;
    }


    std::string serialize_batch() {
      std::string s;
      char hello[9];

      for (auto item : _write_batch) {
        snprintf(hello, sizeof(hello), "%08d", static_cast<int>(item.first.size()));
        hello[8] = '\0';
        s += std::string(hello); 
        snprintf(hello, sizeof(hello), "%08d", static_cast<int>(item.second.size()));
        hello[8] = '\0';
        s += std::string(hello);
        s += item.first;
        s += item.second;
      }

      // std::cout << __PRETTY_FUNCTION__ << " " << s << "\n";
      return s;
    }


    bool compare_write_set(std::string& logged_write_set) {
      // compare _write_batch with logged_write_set
      return true;
    }

    // pointer to the node's db instance (shared between all objects)
    static rocksdb::TransactionDB* ptr_db; 

  private:
    rocksdb::Transaction* txn_handle = nullptr; // pointer to on-going txn

    // default write options for txns
    rocksdb::WriteOptions write_options;
    rocksdb::ReadOptions read_options;

    /* Current's LocalTxn's ID. Note: this Id is different from the RocksDB's ID */
    int txnId;

    // the coordinator's node id and the globalTxn's (as seen from coordinator)
    int tc, tcId;

    /* This is the txnInternalState regarding 2PC */
    int txnState;

    std::unordered_map<std::string, std::string> _write_batch;
};
