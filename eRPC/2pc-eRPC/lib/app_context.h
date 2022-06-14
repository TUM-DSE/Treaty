#pragma once

#include <atomic>
#include <iostream>
#include <unordered_map>
#include <map>
#include <tuple>

#include "rocksdb/db.h"

#include "rpc.h"
#include "commit_log/clog.h"
#include "termination.h"


// Peer-peer connections
struct connection_t {
  bool disconnected = false;  					// True if this session is disconnected
  int session_num = -1;       					// eRPC session number
  size_t session_idx = std::numeric_limits<size_t>::max();  	// Index in conn_vec
};

class AppContext {
  public:

    using NodeId = int;
    using SessionNum = int;
    int node_id = 0;  					// Node's ID; global to the system (taken from configuration)

    std::unordered_map<NodeId, connection_t> cluster_map;
    int cluster_size = 0;
    int nb_fibers = -1;
    int nb_worker_threads = -1;

    struct Nodes_info {
        int node_id = -1;
        std::string hostname;
    };

    std::vector<struct Nodes_info> remote_nodes;

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