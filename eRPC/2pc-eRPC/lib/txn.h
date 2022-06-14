#pragma once

#include <atomic>
#include <iostream>
#include <unordered_map>
#include <map>
#include <tuple>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "db/db_impl.h"
#include "rocksdb/utilities/transaction_db.h"

#include "rpc.h"
#include "sample_operations.h"
#include "transactions_info.h"
#include "debug.h"
#include "util.h"
#include "local_txns.h"
#include "commit_log/clog.h"
#include "termination.h"

#include "safe_functions.h"

/*
 * make use of a thread-safe hash-map
 * @dimitra: folly/boost might not be ported on SCONE easily
 */
#include "HashMap.h" 
#include "stats/stats.h"

#include <boost/fiber/all.hpp>


#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#ifdef SCONE
#include "server_app_scone/message.pb.h"
#else
#include "server_app/message.pb.h"
#endif

#include "app_context.h"

#undef NDEBUG 
#include <cassert>

/** @dimitra TODO: please remove it, we use a map instead.
 * Number of operations of each txn. 
 * It is required because every request must have a unique msgbuffer.
 * 
 * #define TXN_SIZE 	20
 */


static constexpr size_t kMsgSize = 2048;

// Request Types
static constexpr uint8_t kReqTxnPut             = 0;
static constexpr uint8_t kReqTxnRead 		        = 20;
static constexpr uint8_t kReqTxnCommit 		      = 40;
static constexpr uint8_t kReqTxnPrepare 	      = 60;
static constexpr uint8_t kReqTxnRollback 	      = 80;
static constexpr uint8_t kReqTerminate 		      = 120;
static constexpr uint8_t kReqTxnReadForUpdate 	= 100;
static constexpr uint8_t kReqTxnDelete 		      = 140;

/*
 * static constexpr uint8_t kReqTxnBegin = 1;
 * static constexpr uint8_t kReqTxnAbort = 5000;
 * static constexpr uint8_t kReqRecoveredParticipant = 8000;
 * static constexpr uint8_t kReqRecoveredCoordinator = 9000;
 */

#if 0
using NodeId = int;
using SessionNum = int;

// Peer-peer connections
struct connection_t {
  bool disconnected = false;  					// True if this session is disconnected
  int session_num = -1;       					// eRPC session number
  size_t session_idx = std::numeric_limits<size_t>::max();  	// Index in conn_vec
};


class AppContext {
  public:
    int node_id = 0;  					// Node's ID; global to the system (taken from configuration)

    std::unordered_map<NodeId, connection_t> cluster_map;
    int cluster_size = 0;

    // rpc object
    erpc::Rpc<erpc::CTransport> *rpc = nullptr;

    SessionNum getSessionOfNode(NodeId id) {
      assert(id <= cluster_size && "wrong node id");
      return cluster_map[id].session_num;
    }

    /**
     * current's thread ID; the remote thread will use this RID to
     * create a connection to this thread.
     * This id is passed to the rpc object that the current thread
     * creates.
     */
    int RID = -1;

    CommitLog* log;

    CommitLog* create_commit_log(std::string _fname) {
      return new CommitLog(_fname);
    }


    bool all_participants_connected(std::unordered_map<int, int> _map) {
      for (auto i : _map) {
        int session = getSessionOfNode(i.first);
        if (!rpc->is_connected(session))
          return false;
      }

      return true;
    }


    AppContext() = default;

    ~AppContext() {
      // delete log;
    }

    /**
     * Pointer to the underlying rocksdb-5.6-native instance.
     * Required to invoke the coordinator_commit(),
     * coordinator_pre_prepare(), coordinator_post_prepare()
     * methods. These methods append to the clog all 
     * coordinators operations. 
     */
    rocksdb::TransactionDB* rocksdb_ptr = nullptr;


    // @dimitra TODO: document this please
    struct transaction_info_per_fiber {
      // these are per-fiber so we need to distinguish them (an array<fiber_id, data_structure>)
      std::map<int, std::string> read_values;
      std::vector<std::string> get_indexing;
      std::vector<std::string> getForUpdate_indexing;
      int txn_state = 1; /* all good no error found so far */

      /*
       * std::vector<std::string> put_indexing;
       * std::mutex mtx;
       * std::vector<std::string> prepare_indexing;
       * std::vector<std::string> commit_indexing;
       * std::vector<std::string> rollback_indexing;
       * std::map<std::string, erpc::MsgBuffer> response_buffers;
       */
    };

    // each rpc can have up to 128 clients/fibers (?) --- @dimitra make this modular?
    std::array<struct transaction_info_per_fiber*, 128> fibers_txn_data;

};
#endif

/**
 * File-scope thread-safe hash-maps. They keep the number of finished operations per
 * transaction for each global transaction (coordinator) and the state of each on going 
 * transaction.
 */
#if 0
using txnId = int;
using txn_state = int;
using done_operations = int;

static CTSL::HashMap<txnId, done_operations> finished_operations_per_txn;

static CTSL::HashMap<txnId, txn_state> state_per_txn;
#endif
extern std::array<int, 4096> global_fiber_id;

// static int ack;
/**
 * class Txn represents a distributed transaction.
 * Each coordinator has a single object for each transaction.
 * Note: Currently the Txn and TCoordinator classes are 1:1 mapped;
 * to begin a new transaction we need to create a new TCoordinator 
 * object. (FIXME idea: re-initialize the current TCoordinator)
 */

class Txn {
  public:
    Txn() = default;

    Txn(AppContext* _c) : _context(_c), 
    txnId(std::atomic_fetch_add(&txn_ids,1)), 
    txnState(kInProgress), index(0), 
    total_operations(0) {
      assert(_c != nullptr);

      // TODO: consider merging them 
      /*
        struct {
            int finished_ops = 0;
            int recv_prep_msg = 0;
            int commit_acks = -1;
            int txn_state = kInProgress;
        }

       */
      /*
      finished_operations_per_txn.insert(txnId, 0);

      _prep_msg.insert(txnId, 0);

      state_per_txn.insert(txnId, kInProgress);
	*/

      struct txn_infos txn{0, 0 , -1, kInProgress};
      txn_infos_map.insert(txnId, txn);

      /*
      for (int i = 0; i < TXN_SIZE; i++) {
      req.push_back(_context->rpc->alloc_msg_buffer_or_die(kMsgSize));
      req[i] = _context->rpc->alloc_msg_buffer_or_die(kMsgSize);
      }
      */

      currentNodeId = _c->node_id;
    };


    Txn(AppContext* _c, int id) : _context(_c), 
    txnId(id), 
    txnState(kInProgress), index(0), 
    total_operations(0) {
      assert(_c != nullptr);
      /*
      finished_operations_per_txn.insert(txnId, 0);

      _prep_msg.insert(txnId, 0);

      state_per_txn.insert(txnId, kInProgress);
	*/

      struct txn_infos txn{0, 0 , -1, kInProgress};
      txn_infos_map.insert(txnId, txn);
      currentNodeId = _c->node_id;
    };


    ~Txn() {

      for (auto& elem : req) {
        _context->rpc->free_msg_buffer(elem.second);
        elem.second.buf = nullptr;
      }
    };

    int readIndex() {
      return index;
    }

    int gettxnId() { 
      return txnId; 
    }


  private: 
    AppContext* _context;
    int txnId, txnState, index, total_operations, currentNodeId;

    // for each participant keep its current's local txn's id
    using NodeId = int;
    using localTxnId = int;

    // FIXME: change it no need to have the localTxnId
    std::unordered_map<NodeId, localTxnId> glob_to_loc_txns;

    using key = std::string;
    using value = std::string;
    using _write_batch = std::unordered_map<key, value>;

    _write_batch distributed_txn_batch;

  private:
    // helper functions
    bool create_new_local_txn(char* buf) {
      std::string id(buf);
      std::unique_ptr<LocalTxn> new_txn = std::make_unique<LocalTxn>(std::atoi(id.c_str()));
      bool ack = new_txn.get()->beginLocalTxn();
      assert(ack);
      local_txns->insert(id, std::move(new_txn));
      return true;
    }

    std::unique_ptr<char[]> format_index_identifier(int currNodeId, int recipientId, int txnId, int indexOp, int fiber_id) {
      size_t alloc_size = 32 + 8 +  1; // null-terminated string
      std::unique_ptr<char[]> indexing = std::make_unique<char[]>(alloc_size);

      size_t offset = 0;
      char *hello = reinterpret_cast<char*>(indexing.get());
      offset += snprintf(hello+offset, alloc_size-offset, "%08d", currNodeId);
      offset += snprintf(hello+offset, alloc_size-offset, "%08d", recipientId);
      offset += snprintf(hello+offset, alloc_size-offset, "%08d", txnId);
      offset += snprintf(hello+offset, alloc_size-offset, "%08d", indexOp);
      offset += snprintf(hello+offset, alloc_size-offset, "%08d", fiber_id);
      hello[alloc_size - 1] = '\0'; // null-terminated string

      return std::move(indexing); // prevents copy elision but better for std::unique_ptr<>
    };

    static int get_index(const char* msg) {
      char temp[9];
      temp[8] = '\0';
      ::memcpy(temp, msg+24, 8);
      return std::atoi(temp);

    }


    std::string serialize_batch(std::unordered_map<std::string, std::string> _map) {
      std::string s;
      char buf[9];

      for (auto item : _map) {
        snprintf(buf, sizeof(buf), "%08d", static_cast<int>(item.first.size()));
        buf[8] = '\0';
        s += std::string(buf);
        snprintf(buf, sizeof(buf), "%08d", static_cast<int>(item.second.size()));
        buf[8] = '\0';
        s += std::string(buf);

        s += item.first;
        s += item.second;
      }
      // std::cout << s << "\n";
      // print_debug(s);

      return s;
    };


  public:
    // this is for assigning ids to a txn of a particular server
    static std::atomic<int> txn_ids;
    // erpc::MsgBuffer req[TXN_SIZE];
    // std::vector<erpc::MsgBuffer> req;
    std::map<int, erpc::MsgBuffer> req;

    // shared between all distributed transactions
    // static CTSL::HashMap<std::string, std::unique_ptr<erpc::MsgBuffer>> pool_resp;
    static CTSL::HashMap<std::string, erpc::MsgBuffer> pool_resp;
    static CTSL::HashMap<std::string, std::string> index_table;

    int state() {
      return txnState;
    };

    int returnTransactionId() {
      return txnId;
    };

    bool IAmTheNode(int participantNodeId) {
      return (participantNodeId == _context->node_id);
    };

    static void _print_address (void* p) {
      fprintf(stdout, "%p\n", p);
    }

    static void cont_func_terminate(void*, void*) {
      return;
    };


    static void cont_func_txnDelete(void* context, void *tag) {
      assert(context != nullptr);
      auto* _c = static_cast<AppContext *>(context);
      auto* _tag = static_cast<std::string*>(tag);

      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }

      int txn_id = extractTxnId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);
      int fiber_id = extractFiberId(*_tag);

#ifdef ENCRYPTION
//#if 0
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      auto enc_size = msg.get_data_size();
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());

#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
      size_t size = msg.get_data_size();
#endif

      if (success(__msg)) {
        // 
      }
      else {
        _c->fibers_txn_data[fiber_id]->txn_state = 0;
      }

     
      auto txn_info = txn_infos_map.find(txn_id);
      txn_info->finished_ops += 1;
     
      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

     
      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };

    static void cont_func_txnPut(void* context, void *tag) {
      assert(context != nullptr);
      auto* _c = static_cast<AppContext *>(context);
      auto* _tag = static_cast<std::string*>(tag);
     
      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }

      int txn_id = extractTxnId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);
      int fiber_id = extractFiberId(*_tag);

#ifdef ENCRYPTION
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      size_t enc_size = msg.get_data_size();         
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());
#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
      size_t size = msg.get_data_size();
#endif

      if (success(__msg)) {
        // 
      }
      else {
        _c->fibers_txn_data[fiber_id]->txn_state = 0;
      }

      auto txn_info = txn_infos_map.find(txn_id);
      txn_info->finished_ops += 1;

      // printf("[Node %d] received put-response %s (data_size = %lu) - %p (from Node %d) with index %s for txn %d -- fiber_id %d finished_ops = %d\n", _c->node_id, __msg, size, __msg,  participant_node, _tag->c_str(), txn_id, (fiber_id), txn_info->finished_ops);

      
      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };

    static size_t convertByteArrayToInt(char* b) {
      return (b[0] << 24)
        + ((b[1] & 0xFF) << 16)
        + ((b[2] & 0xFF) << 8)
        + (b[3] & 0xFF);
    }

    static void cont_func_txnReadForUpdate(void* context, void* tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);

      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }
      int txn_id = extractTxnId(*_tag);
      int fiber_id = extractFiberId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);

#ifdef ENCRYPTION
//#if 0
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      size_t enc_size = msg.get_data_size();
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());

#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
#endif
      // printf("[Node %d] received readforUpdate-response %s - %p (data size : %lu) (from Node %d) with index %s for txn %d with fiber_id %d\n", id, __msg, __msg, msg.get_data_size(),  participant_node, _tag->c_str(), txn_id, fiber_id);


      if (success(__msg)) {
        auto value_size =  convertByteArrayToInt(__msg+7);
        std::unique_ptr<char []> value_ptr = std::make_unique<char []>(value_size + 1);
        value_ptr[value_size] = '\0';
        ::memcpy(value_ptr.get(), __msg+7+4, value_size);
        auto order = get_index(_tag->c_str());
        
        std::string _st(value_ptr.get(), value_size); 
        _c->fibers_txn_data[fiber_id]->read_values.insert({order, _st});
      }
      else {
        std::cout << "no success()\n";
        _c->fibers_txn_data[fiber_id]->txn_state = 0;
      }


      auto txn_info = txn_infos_map.find(txn_id);
      txn_info->finished_ops += 1;
      

      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

      _c->fibers_txn_data[fiber_id]->getForUpdate_indexing.erase(_c->fibers_txn_data[fiber_id]->getForUpdate_indexing.begin());

      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };


    static void cont_func_txnRead(void* context, void* tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);
   
      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }
      int txn_id = extractTxnId(*_tag);
      int fiber_id = extractFiberId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);

#ifdef ENCRYPTION
// #if 0
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      size_t enc_size = msg.get_data_size();
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());

#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
#endif

      if (success(__msg)) {
        auto value_size =  convertByteArrayToInt(__msg+7);
        std::unique_ptr<char []> value_ptr = std::make_unique<char []>(value_size + 1);
        value_ptr[value_size] = '\0';
        ::memcpy(value_ptr.get(), __msg+7+4, value_size);
        auto order = get_index(_tag->c_str());
      
        std::string _st(value_ptr.get(), value_size);
       
        _c->fibers_txn_data[fiber_id]->read_values.insert({order, _st});
      }
      else {
        std::cout << "no success()\n";
        _c->fibers_txn_data[fiber_id]->txn_state = 0;
      }

      
      auto txn_info = txn_infos_map.find(txn_id);
      txn_info->finished_ops += 1;
      // printf("[Node %d] received read-response %s\n (data size = %lu) - %p (from Node %d) with index %s for txn %d with fiber_id %d -- finished_ops = %d\n", id, __msg, msg.get_data_size(), __msg, participant_node, _tag->c_str(), txn_id, fiber_id, txn_info->finished_ops);
     
      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

      _c->fibers_txn_data[fiber_id]->get_indexing.erase(_c->fibers_txn_data[fiber_id]->get_indexing.begin());
      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };


    static void cont_func_txnPrepare(void* context, void *tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);
      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }
      int txn_id = extractTxnId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);

#ifdef ENCRYPTION 
// #if 0
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      size_t enc_size = msg.get_data_size();         
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());

#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
#endif

      if (!success(__msg)) {
        auto db = static_cast<rocksdb::DBImpl*>(_c->rocksdb_ptr->GetRootDB());
 //       db->coordinator_post_prepare(_c->RID, _c->node_id, txn_id, 0);
      }


      auto txn_info = txn_infos_map.find(txn_id);
     
      txn_info->recv_prep_msg -= 1;
      if (txn_info->recv_prep_msg == 0) {
        auto db = static_cast<rocksdb::DBImpl*>(_c->rocksdb_ptr->GetRootDB());
        db->coordinator_post_prepare(_c->RID, _c->node_id, txn_id, 1);
      }

      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };


    static void cont_func_txnRecovered(void* context, void* tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);
      assert(tag != nullptr && _tag != nullptr);
      // erpc::MsgBuffer msg;
      // pool_resp.find(*_tag, msg);
      // pool_resp.erase(*_tag);
      // fprintf(stdout, "[Node %d] received response %s with index %s\n", id, msg.buf, _tag->c_str());
      // ack = 1;
    };


    static void cont_func_txnRollback(void* context, void *tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);

      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }

      int txn_id = extractTxnId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);

// #if 0
#ifdef ENCRYPTION 
      char* enc_data = reinterpret_cast<char*>(msg.buf);
      size_t enc_size = msg.get_data_size();         
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());

#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
#endif
      printf("[Node %d] received rollback-response %s (from Node %d) with index %s for txn %d\n", id, __msg, participant_node, _tag->c_str(), txn_id);
      pool_resp.erase(*_tag);
      index_table.erase(*_tag);

      auto txn_info = txn_infos_map.find(txn_id);
      if ((txn_info->commit_acks-1) == 0) {
        txn_info->txn_state = kAborted;
        txn_info->commit_acks -= 1;
      }
      else {
        txn_info->commit_acks -= 1;
      }

      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };

    static void cont_func_txnCommit(void* context, void *tag) {
      auto* _c = static_cast<AppContext *>(context);
      int id = static_cast<int>(_c->node_id);
      auto* _tag = static_cast<std::string*>(tag);

      assert(tag != nullptr && _tag != nullptr);

      erpc::MsgBuffer msg;
      pool_resp.find(*_tag, msg);
      if (msg.buf == nullptr) {
        exit(122);
      }

      int txn_id = extractTxnId(*_tag);
      int participant_node = extractParticipantNodeId(*_tag);

// #if 0
#ifdef ENCRYPTION
      char* enc_data = reinterpret_cast<char*>(msg.buf);   
      size_t enc_size = msg.get_data_size();         
      std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
      char* __msg = reinterpret_cast<char*>(dec_msg.get());
#else
      char* __msg = reinterpret_cast<char*>(msg.buf);
#endif
  
      pool_resp.erase(*_tag);
      index_table.erase(*_tag);


      auto txn_info = txn_infos_map.find(txn_id);
      if ((txn_info->commit_acks-1) == 0) {
        txn_info->txn_state = kCommitted;
        txn_info->commit_acks -= 1;
      }
      else {
        txn_info->commit_acks -= 1;
      }

      _c->rpc->free_msg_buffer(msg);
      msg.buf = nullptr;
    };


    static int extractTxnId(std::string _tag) {
      const char* src = _tag.c_str();
      char buf[9];
      ::memcpy(buf, src+16, 8);
      buf[8] = '\0';
      int id = std::atoi(buf);
      return id;
    };


    static int extractParticipantNodeId(std::string _tag) {
      const char* src = _tag.c_str();
      char buf[9];
      ::memcpy(buf, src+8, 8);
      buf[8] = '\0';
      int id = std::atoi(buf);
      return id;
    };


    static int extractFiberId(std::string _tag) {
      const char* src = _tag.c_str();
      char buf[9];
      ::memcpy(buf, src+32, 8);
      buf[8] = '\0';
      int id = std::atoi(buf);
      return id;
    };


    /**
     * The exposed API for a distributed transaction.
     */

    void AskRecoveredTxn(int globalTxnId, int session, int txn_coordinator, std::vector<std::tuple<std::string, std::string, std::string>> operations_set, bool normal) {
      std::cout << __PRETTY_FUNCTION__ << " txn id: " << globalTxnId << " txn_coordinator: " << txn_coordinator << " \n";
      for (auto op : operations_set) {
        std::string op_type = std::get<0>(op), key = std::get<1>(op), value = std::get<2>(op);
        std::cout << "Iterate over operations_set: " << op_type << " " << key << " " << value << "\n";
        if (op_type == "W") {
          // construct write set for session
        }
        else {
          // construct read set for session
        }
      }


      char hello[1024];
      size_t offset =0;
      int _index = 0;


      offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_coordinator);
      offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", globalTxnId);
      offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", _context->node_id);

      ::memcpy(req[_index].buf, hello, 24);
      if (normal) 
        ::memcpy(req[_index].buf, "OK", 2);

      std::string indexing(hello);

      erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(kMsgSize);

      pool_resp.insert(indexing, erpc_buf);
      erpc::MsgBuffer* resp = pool_resp.find(indexing);

      index_table.insert(indexing, indexing);

      session = _context->getSessionOfNode(0);
      // std::cout << __PRETTY_FUNCTION__ << " " << hello << "\n";
      // _context->rpc->enqueue_request(session, kReqRecoveredCoordinator, &req[_index], resp, cont_func_txnRecovered, (void*) index_table.find(static_cast<std::string>(indexing)));

      bool ack = false;
      while (!ack) {
        _context->rpc->run_event_loop(200);
        if (ctrl_c_pressed.load()) 
          return;
      }

      // ack = 0;
    };


    bool run_event_loop(uint64_t& threshold, uint64_t& nb_iterations) {
      for (int i = 0; i < 10; i++) {
        _context->rpc->run_event_loop_once();
      }
      if (threshold > 0) 
        threshold--;
      else {
        // threshold is 0
        nb_iterations++;
      }

      if (nb_iterations > 1000000) {
        nb_iterations = 0;
        threshold = 10000;
        return false;
      }
      return true;
    }

   
    bool Commit(int fiber_id) {
      int node_id = 0, session, _index = 0;

      
      assert(txnState == kPrepared);
   
      
      auto txn_info = txn_infos_map.find(txnId);
      auto recv_prep_msgs = txn_info->recv_prep_msg;
      
      uint64_t threshold = 10000;
      uint64_t nb_iterations = 0;
      
      // wait for all prepared messages to arrive
      while (recv_prep_msgs != 0) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->finished_ops << " " << total_operations << " " << txn_info->recv_prep_msg << "\n";
        }
        boost::this_fiber::yield();
        txn_info = txn_infos_map.find(txnId);
        recv_prep_msgs = txn_info->recv_prep_msg;
      }


      txn_info->commit_acks = glob_to_loc_txns.size();


      for (auto localTxn : glob_to_loc_txns) {
        // either execute the commit locally or instruct the participant node to commit
        node_id = localTxn.first;
        if (IAmTheNode(node_id)) {
          char buf[1024];
          size_t offset = 0;
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", node_id);
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);
          if (transaction_exists(buf)) {
            std::string id(buf);
            LocalTxn *txn = get_transaction(id);
                   
            bool ack = txn->commitLocalTxn(_context->log, _context->RID, node_id, txnId);
            local_txns->erase(id);

            txn_info = txn_infos_map.find(txnId);
            txn_info->commit_acks -= 1;
            if (txn_info->commit_acks == 0) {
                txn_info->txn_state = kCommitted;
            }
            continue;
          }
          print_local_txns();
          assert(false && "Commit:Transaction does not exist");
        }

        session = _context->getSessionOfNode(node_id);

        std::string _buffer = msg::TxnCommitMsg(txnId, currentNodeId);

        size_t msg_size = _buffer.size();
        
        char* _msg = const_cast<char*>(_buffer.c_str());
// #if 0
#ifdef ENCRYPTION
        {
          size_t enc_size = sizeof_encrypted_buffer(msg_size);
          req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
	        _context->rpc->resize_msg_buffer(&req[_index], enc_size);
	        bool ok = txn_cipher->encrypt(req[_index].buf, _msg, msg_size);
        }
#else
        req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
        ::memcpy(req[_index].buf, _msg, msg_size);
#endif
        auto _findex = format_index_identifier(_context->node_id, node_id, txnId, index, fiber_id);
        const std::string indexing(reinterpret_cast<char*>(_findex.get()), 41);

        erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));
        pool_resp.insert(indexing, erpc_buf);
        erpc::MsgBuffer* resp = pool_resp.find(indexing);
        if (resp->buf == nullptr) {
          exit(122);
        }

#if 0
        _print_address(&req[_index]);
        _print_address(req[_index].buf);
        _print_address(resp);
        _print_address(resp->buf);
#endif

        index_table.insert(indexing, indexing);
        
        _context->rpc->enqueue_request(session, kReqTxnCommit, &req[_index], resp, cont_func_txnCommit, (void*) index_table.find(indexing));
        _context->rpc->run_event_loop_once();
        _index++;
        index++;
      }


      // wait until all nodes commit this txn
      txn_info = txn_infos_map.find(txnId);
      int _state = txn_info->txn_state;

      threshold = 10000;
      nb_iterations = 0;
     
      while (_state != kCommitted) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->finished_ops << " " << total_operations << " " << txn_info->recv_prep_msg <<  " " << txn_info->commit_acks <<"\n";
        }
        boost::this_fiber::yield();
        txn_info = txn_infos_map.find(txnId);
        _state = txn_info->txn_state;
      }
      
      txn_infos_map.erase(txnId);
      txnState = kCommitted;

      auto db = static_cast<rocksdb::DBImpl*>(_context->rocksdb_ptr->GetRootDB());
      db->coordinator_commit(_context->RID, _context->node_id, txnId, 1 /* FIXME */);

      return true;
    };



    void wait_for_all() {
      uint64_t threshold = 10000;
      uint64_t nb_iterations = 0;
      auto txn_info = txn_infos_map.find(txnId);
      auto ops = txn_info->finished_ops;
      while (ops != total_operations) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->finished_ops << " " << total_operations << "\n";
        }
        boost::this_fiber::yield();
        txn_info = txn_infos_map.find(txnId);
	ops = txn_info->finished_ops;
      }
    }

    bool Prepare(int fiber_id) {
      /** 
       * A TCoordinator knows exactly how many operations a specific txn must have.
       * We are waiting until all participants respond for all sent operations.
       * Note: if the message is lost the application will block here (FIXME).
       */
      uint64_t threshold = 10000;
      uint64_t nb_iterations = 0;
      auto txn_info = txn_infos_map.find(txnId);
      while (txn_info->finished_ops != total_operations) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->finished_ops << " " << total_operations << "\n";
        }
        boost::this_fiber::yield();
      }

      int participant_id, session, _index = 0;
      std::string _buffer;

      auto db = static_cast<rocksdb::DBImpl*>(_context->rocksdb_ptr->GetRootDB());
      std::string __batch = serialize_batch(distributed_txn_batch);
      // db->coordinator_pre_prepare(_context->RID, _context->node_id, txnId, distributed_txn_batch.size(), __batch.c_str(), __batch.size());
      
     
      txn_info = txn_infos_map.find(txnId);
      txn_info->recv_prep_msg = glob_to_loc_txns.size();

      for (auto localTxn : glob_to_loc_txns) {
        participant_id = localTxn.first;
        if (IAmTheNode(participant_id)) {
          char buf[1024];
          size_t offset = 0;
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", participant_id);
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);
          if (transaction_exists(buf)) {
            std::string id(buf);
            LocalTxn *txn = get_transaction(id);
            assert(txn && "txn is nullptr");
            bool ack = txn->prepareLocalTxn(_context->log, _context->RID, participant_id, txnId);
            if (!ack)
		          txn_info->txn_state = kAborted;
            
            txn_info->recv_prep_msg -= 1;
            }
          else {
            std::cout << buf << "\n";
            print_local_txns();
            assert(false && "Prepare:Transaction does not exist");
          }
          continue;
        }
        session = _context->getSessionOfNode(participant_id);
        _buffer = msg::TxnPrepMsg(txnId, currentNodeId);
        size_t msg_size = _buffer.size();
        char* _msg = const_cast<char*>(_buffer.c_str());

#ifdef ENCRYPTION
// #if 0
        size_t enc_size = sizeof_encrypted_buffer(msg_size);
        req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
        _context->rpc->resize_msg_buffer(&req[_index], enc_size);
	      bool ok = txn_cipher->encrypt(req[_index].buf, _msg, msg_size);
#else
        req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
        ::memcpy(req[_index].buf, _msg, msg_size);
#endif

        auto _findex = format_index_identifier(_context->node_id, participant_id, txnId, index, fiber_id);
        std::string indexing(_findex.get(), 41);

        erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));

        pool_resp.insert(indexing, erpc_buf);
        erpc::MsgBuffer* resp = pool_resp.find(indexing);

        index_table.insert(indexing, indexing);
#if 0
        _print_address(&req[_index]);
        _print_address(req[_index].buf);
        _print_address(resp);
        _print_address(resp->buf);
#endif
        _context->rpc->enqueue_request(session, kReqTxnPrepare, &req[_index], resp, cont_func_txnPrepare, (void*) index_table.find(indexing));
        _index++;
        index++;
        _context->rpc->run_event_loop_once();
      }

      
      // wait for all prepare-acks to arrive
      threshold = 10000;
      nb_iterations = 0;
      txn_info = txn_infos_map.find(txnId);
      auto recv_prep_msg = txn_info->recv_prep_msg;
      while (recv_prep_msg > 0) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->finished_ops << " " << total_operations << "\n";
        }
        boost::this_fiber::yield();
        txn_info = txn_infos_map.find(txnId);
        recv_prep_msg = txn_info->recv_prep_msg;  
      }

      txnState = kPrepared;
      
      txn_info->txn_state = kPrepared;

#ifdef ROLLBACK_PROTECTION
      boost::this_fiber::sleep_for(std::chrono::milliseconds(2));
#endif
      return true;
    };


    // assumes that wait_for_all() has been executed
    bool Rollback(int fiber_id) {
      int node_id = 0, session, _index = 0;
	    auto txn_info = txn_infos_map.find(txnId);
      txn_info->commit_acks = glob_to_loc_txns.size();

      for (auto localTxn : glob_to_loc_txns) {
        node_id = localTxn.first;
        if (IAmTheNode(node_id)) {
          char buf[1024];
          size_t offset = 0;
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", node_id);
          offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);
          if (transaction_exists(buf)) {
            std::string id(buf);
            LocalTxn *txn = get_transaction(id);
            assert(txn && "txn is nullptr");
            bool ack = txn->RollbackLocalTxn();
            txn_info = txn_infos_map.find(txnId);
            txn_info->commit_acks -= 1;

            if (txn_info->commit_acks == 0) {
                txn_info->txn_state = kAborted;
            }
  
            local_txns->erase(id);
            continue;
          }
          print_local_txns();
          assert(false && "Commit:Transaction does not exist");
        }
        session = _context->getSessionOfNode(node_id);

        std::string _buffer = msg::TxnCommitMsg(txnId, currentNodeId);

        size_t msg_size = _buffer.size();
        char* _msg = const_cast<char*>(_buffer.c_str());

 #ifdef ENCRYPTION 
//#if 0 
        {
          size_t enc_size = sizeof_encrypted_buffer(msg_size);
          req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
          _context->rpc->resize_msg_buffer(&req[_index], enc_size);
	        txn_cipher->encrypt(req[_index].buf, _msg, msg_size);
        }
#else

        req.insert({_index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
        _context->rpc->resize_msg_buffer(&req[_index], msg_size);
        ::memcpy(req[_index].buf, _msg, msg_size);
#endif
        auto _findex = format_index_identifier(_context->node_id, node_id, txnId, index, fiber_id);
        const std::string indexing(reinterpret_cast<char*>(_findex.get()), 41);

        erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));
        pool_resp.insert(indexing, erpc_buf);
        erpc::MsgBuffer* resp = pool_resp.find(indexing);

#if 0
        _print_address(&req[_index]);
        _print_address(req[_index].buf);
        _print_address(resp);
        _print_address(resp->buf);
#endif

        index_table.insert(indexing, indexing);

        _context->rpc->enqueue_request(session, kReqTxnRollback, &req[_index], resp, cont_func_txnRollback, (void*) index_table.find(indexing));
        _context->rpc->run_event_loop_once();
        _index++;
        index++;
      }


      // wait until all participants acknowledged the rollback
      uint64_t threshold = 10000;
      uint64_t nb_iterations = 0;
      txn_info = txn_infos_map.find(txnId);
      int _state = txn_info->txn_state;
      while (_state != kAborted) {
        if (!run_event_loop(threshold, nb_iterations)) {
          std::cout << txnId << " " << __PRETTY_FUNCTION__ << " " << txn_info->txn_state << " " << txn_info->commit_acks << "\n";
        }
        boost::this_fiber::yield();
        txn_info = txn_infos_map.find(txnId);
        _state = txn_info->txn_state;
      }
      
      txn_infos_map.erase(txnId);
      txnState = kAborted;

      auto db = static_cast<rocksdb::DBImpl*>(_context->rocksdb_ptr->GetRootDB());
      db->coordinator_commit(_context->RID, _context->node_id, txnId, 1 /* FIXME */);

      std::cout << __PRETTY_FUNCTION__ << " txn_id: " << txnId << "\n";
      return true;
    };


    bool Delete(std::string& key, int _NodeId, int SessionNum, int fiber_id) { 
      distributed_txn_batch[key] = " ";
      if (IAmTheNode(_NodeId)) {
        total_operations++;
        glob_to_loc_txns[_NodeId] = -1;
        char buf[1024];
        size_t offset = 0;
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", _NodeId);
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);
        if (!transaction_exists(buf)) {
          create_new_local_txn(buf);
        }

        std::string id(buf);
        LocalTxn *txn = get_transaction(id);
        assert(txn && "txn is nullptr");
        bool ack = txn->deleteLocalTxn(key);
        index++;
        if (ack) {
          // success
        }
        else {
          txnState = kToBeAborted;
          _context->fibers_txn_data[fiber_id]->txn_state = 0;
        }
      
        auto txn_info = txn_infos_map.find(txnId);
        txn_info->finished_ops += 1;
        
        return ack;
      }


      // enqueue_request for deleting a key
      std::unique_ptr<char[]> buffer = msg::TxnDeleteMsg(key, txnId, currentNodeId);
      size_t msg_size = sizeof(char) + 24 + key.size() + 1;
      const char* _msg = buffer.get();


#ifdef ENCRYPTION
//#if 0
      size_t enc_size = sizeof_encrypted_buffer(msg_size);
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
      _context->rpc->resize_msg_buffer(&req[index], enc_size);
      bool ok = txn_cipher->encrypt(req[index].buf, _msg, msg_size);

#else
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
      _context->rpc->resize_msg_buffer(&req[index], msg_size);
      ::memcpy(req[index].buf, _msg, msg_size);
#endif
      auto _findex = format_index_identifier(_context->node_id, _NodeId, txnId, index, fiber_id);
      std::string indexing(_findex.get(), 41);

      erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));
      pool_resp.insert(indexing, erpc_buf);
      erpc::MsgBuffer* resp = pool_resp.find(indexing);

      assert(SessionNum >=0);
      assert(resp != nullptr);


      index_table.insert(indexing, indexing);
      void* ptr = (void*) index_table.find(indexing);

      _context->rpc->enqueue_request(SessionNum, kReqTxnDelete, &req[index], resp, cont_func_txnDelete, (void*) (ptr));
      _context->rpc->run_event_loop_once();
      total_operations++;
      index++;
      glob_to_loc_txns[_NodeId] = -1;
      return true; 
    };


    bool Put(std::string& key, std::string& value, int _NodeId, int SessionNum, int fiber_id) { 
      distributed_txn_batch[key] = value;
      
      if (IAmTheNode(_NodeId)) {
        glob_to_loc_txns[_NodeId] = -1;
	total_operations++;
        char buf[1024];
        size_t offset = 0;
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", _NodeId);
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);
        if (!transaction_exists(buf)) {
          create_new_local_txn(buf);
        }

        std::string id(buf);
        LocalTxn *txn = get_transaction(id);
        assert(txn && "txn is nullptr");
        bool ack = txn->putLocalTxn(key, value);
        index++;
        if (ack) {
          // success
        }
        else {
          txnState = kToBeAborted;
          _context->fibers_txn_data[fiber_id]->txn_state = 0;
        }
      
        auto txn_info = txn_infos_map.find(txnId);
        txn_info->finished_ops += 1;

      	// std::cout << __PRETTY_FUNCTION__ << " " << txnId << " " << txn_info->finished_ops << "\n";
        return ack;
      }

      std::unique_ptr<char[]> buffer = msg::TxnPutMsg(key, value, txnId, currentNodeId);
      size_t msg_size = sizeof(char) + 32 + key.size() + value.size() + 1;
      const char* _msg = buffer.get();

#ifdef ENCRYPTION 
      size_t enc_size = sizeof_encrypted_buffer(msg_size);
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
      _context->rpc->resize_msg_buffer(&req[index], enc_size);
	    bool ok = txn_cipher->encrypt(req[index].buf, _msg, msg_size);

#else
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
      _context->rpc->resize_msg_buffer(&req[index], msg_size);
      ::memcpy(req[index].buf, _msg, msg_size);
#endif
      auto _findex = format_index_identifier(_context->node_id, _NodeId, txnId, index, fiber_id);
      std::string indexing(_findex.get(), 41);

      erpc::MsgBuffer erpc_buf = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));
      pool_resp.insert(indexing, erpc_buf);
      erpc::MsgBuffer* resp = pool_resp.find(indexing);

      assert(SessionNum >=0);
      assert(resp != nullptr);


      index_table.insert(indexing, indexing);
      void* ptr = (void*) index_table.find(indexing);

      _context->rpc->enqueue_request(SessionNum, kReqTxnPut, &req[index], resp, cont_func_txnPut, (void*) (ptr));
      _context->rpc->run_event_loop_once();
      total_operations++;
      index++;
      glob_to_loc_txns[_NodeId] = -1;
      return true; 
    };


    int ReadForUpdate(std::string& key, std::string& value, int NodeId, int SessionNum, int fiber_id) { 
      if (IAmTheNode(NodeId)) {
        total_operations++;
        glob_to_loc_txns[NodeId] = -1;
        char buf[1024];
        size_t offset = 0;
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", NodeId);
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);

        if (!transaction_exists(buf)) {
          create_new_local_txn(buf);
        }

        std::string id(buf);
        LocalTxn *txn = get_transaction(id);
        assert(txn && "txn is nullptr");

        int ack = txn->readForUpdateLocalTxn(key, &value);
        
        index++;
        
        auto txn_info = txn_infos_map.find(txnId);
        txn_info->finished_ops += 1;

        return ack;
      }

      
      std::string buffer = msg::TxnReadMsg(key, txnId, currentNodeId);
      size_t msg_size = buffer.size();
      char *_msg = const_cast<char*>(buffer.c_str());

#ifdef ENCRYPTION 
// #if 0
      size_t enc_size = sizeof_encrypted_buffer(msg_size);
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
      _context->rpc->resize_msg_buffer(&req[index], enc_size);
      bool ok = txn_cipher->encrypt(req[index].buf, _msg, msg_size);
#else
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
      _context->rpc->resize_msg_buffer(&req[index], msg_size);
      ::memcpy(req[index].buf, _msg, msg_size);
#endif

      erpc::MsgBuffer msg = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));

      auto _findex = format_index_identifier(_context->node_id, NodeId, txnId, index, fiber_id);
      std::string indexing(_findex.get(), 41);

      pool_resp.insert(indexing, msg);
      erpc::MsgBuffer* resp = pool_resp.find(indexing);


      index_table.insert(indexing, indexing);


      assert(resp != nullptr);

      void* ptr = (void*) index_table.find(indexing);
      _context->fibers_txn_data[fiber_id]->getForUpdate_indexing.push_back(indexing);
      
      _context->rpc->enqueue_request(SessionNum, kReqTxnReadForUpdate, &req[index], resp, cont_func_txnReadForUpdate, (void*) (ptr));
      _context->rpc->run_event_loop_once();

      total_operations++;
      index++;
      glob_to_loc_txns[NodeId] = -1;
      return 2; 
    };


    int Read(std::string& key, std::string& value, int NodeId, int SessionNum, int fiber_id) {
      if (IAmTheNode(NodeId)) {
        total_operations++;
        glob_to_loc_txns[NodeId] = -1;
        char buf[1024];
        size_t offset = 0;
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", NodeId);
        offset += snprintf(buf+offset, sizeof(buf)-offset, "%08d", txnId);

        if (!transaction_exists(buf)) {
          create_new_local_txn(buf);
        }

        std::string id(buf);
        LocalTxn *txn = get_transaction(id);
        assert(txn && "txn is nullptr");

        int ack = txn->readLocalTxn(key, &value);
        index++;

        auto txn_info = txn_infos_map.find(txnId);
        txn_info->finished_ops += 1;
      	// std::cout << __PRETTY_FUNCTION__ << " " << txnId << " " << txn_info->finished_ops << "\n";
        return ack;
      }
	
      std::string buffer = msg::TxnReadMsg(key, txnId, currentNodeId);
      size_t msg_size = buffer.size();
      char *_msg = const_cast<char*>(buffer.c_str());


#ifdef ENCRYPTION 
// #if 0
      size_t enc_size = sizeof_encrypted_buffer(msg_size);
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(enc_size)});
      _context->rpc->resize_msg_buffer(&req[index], enc_size);
      bool ok = txn_cipher->encrypt(req[index].buf, _msg, msg_size);

#else
      req.insert({index, _context->rpc->alloc_msg_buffer_or_die(msg_size)});
      ::memcpy(req[index].buf, _msg, msg_size);
#endif

      erpc::MsgBuffer msg = _context->rpc->alloc_msg_buffer_or_die(sizeof_encrypted_buffer(kMsgSize));

      auto _findex = format_index_identifier(_context->node_id, NodeId, txnId, index, fiber_id);
      std::string indexing(_findex.get(), 41);

      pool_resp.insert(indexing, msg);
      erpc::MsgBuffer* resp = pool_resp.find(indexing);


      index_table.insert(indexing, indexing);


      assert(resp != nullptr);

      void* ptr = (void*) index_table.find(indexing);
      _context->fibers_txn_data[fiber_id]->get_indexing.push_back(indexing);

      _context->rpc->enqueue_request(SessionNum, kReqTxnRead, &req[index], resp, cont_func_txnRead, (void*) (ptr));
      _context->rpc->run_event_loop_once();

      total_operations++;
      index++;
      glob_to_loc_txns[NodeId] = -1;
      return 2; 
    };

    void Terminate(int SessionNum) {
      // LOG_DEBUG(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      _context->rpc->enqueue_request(SessionNum, kReqTerminate, &req[index], &req[index+1], cont_func_terminate, nullptr);
      _context->rpc->run_event_loop(200);
    };

};
