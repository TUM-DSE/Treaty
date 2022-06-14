#include <signal.h>
#include <cerrno>
#include <cstring>
#include <memory>

#include <sys/types.h>
#include <unistd.h>

#include "recovery/recover.h"
#include "request_handlers.h"
#include "util.h"
#include "txn.h"
#include "db.h"
#include "termination.h"

#include "safe_functions.h"

#include "encrypt_package.h"

#include "stats/stats.h"


#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#ifdef SCONE
#include "server_app_scone/message.pb.h"
#else
#include "server_app/message.pb.h"
#endif

extern std::atomic<bool> ctrl_c_pressed;

extern CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>* local_txns;

extern std::shared_ptr<PacketSsl> txn_cipher;

extern int participant_recovered;


static void convertIntToByteArray(char* dst, int sz) {
  auto tmp = dst;
  tmp[0] = (sz >> 24) & 0xFF;
  tmp[1] = (sz >> 16) & 0xFF;
  tmp[2] = (sz >> 8) & 0xFF;
  tmp[3] = sz & 0xFF;
}


// @dimitra: handler for session management
void sm_handler(int session_num, erpc::SmEventType, erpc::SmErrType, void *) {
	std::cout << __PRETTY_FUNCTION__ << " (session_num: " << session_num << "\n";
}


void req_handler_txnRollback(erpc::ReqHandle *req_handle, void *context) {
  auto* app_context = static_cast<AppContext *>(context);
  auto & _resp = req_handle->dyn_resp_msgbuf;
  
  bool ack = false;
 
#ifdef ENCRYPTION
// #if 0
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get(), 16);
  char* buf = _buf.get();

#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::unique_ptr<char[]> _buf = safe_memcpy(msg, 16);
  char* buf = _buf.get();
#endif

  if (transaction_exists(buf)) {
    std::string id(buf);
    LocalTxn *txn = get_transaction(id);
    // ack = txn->RollbackLocalTxn(_c->log, _c->RID, _c->node_id, std::atoi(id.c_str()));
    ack = txn->RollbackLocalTxn();
    // TODO: FIXME: response message!
    ack = true;

#ifdef ENCRYPTION
    if (ack) {
      std::string st("RollbackACK_ENCRYPTION");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;
      st += '\0';
      auto msg_size = st.size();

      if (msg_size >= 800) {
        _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }
    else {
      std::string st("RollbackFAIL");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;
	    st += '\0';
      auto msg_size = st.size();

      if (msg_size >= 800) {
        _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }
#else
    if (ack) {
      std::string st("RollbackACK");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;

      auto msg_size = st.size() + 1;

      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
      
      
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }
    else {
      std::string st("RollbackFAIL");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;

      auto msg_size = st.size() + 1;

      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
      
      
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }
#endif
    app_context->rpc->enqueue_response(req_handle, &_resp);

    local_txns->erase(id);
    return;
  }
  print_local_txns();
  assert(false && "Commit:Transaction does not exist");
}

// invoked when receiving a message of kReqTxnCommit
void req_handler_txnCommit(erpc::ReqHandle *req_handle, void *context) {
  auto* app_context = static_cast<AppContext *>(context);
  auto & _resp = req_handle->dyn_resp_msgbuf;
  bool ack = false;
  
// #if 0
#ifdef ENCRYPTION
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get(), 16);
  char* buf = _buf.get();
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::unique_ptr<char[]> _buf = safe_memcpy(msg, 16);
  char* buf = _buf.get();
#endif
  if (transaction_exists(buf)) {
    /**
     * sub-transaction exists; execute the requested operation and
     * respond back whether it was successful or not.
     */
    std::string id(buf);
    LocalTxn *txn = get_transaction(id);
    ack = txn->commitLocalTxn(app_context->log, app_context->RID, app_context->node_id, std::atoi(id.c_str()));
    // TODO: FIXME: response message!
    ack = true;

#ifdef ENCRYPTION
// #if 0
    if (ack) {
      std::string st("CommitACK_ENCRYPTION");
      st += std::to_string(app_context->node_id);
      st += '\0';
      auto msg_size = st.size();

      if (msg_size >= 800) {
        _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }  
    else {
      std::string st("CommitFAIL_ENCRYPTION");
      st += std::to_string(app_context->node_id);
      st += '\0';

      auto msg_size = st.size();

      if (msg_size >= 800) {
        _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }

#else
    if (ack) {
      std::string st("CommitACK_COMMIT");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;

      auto msg_size = st.size() + 1;

      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
      
      
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }  
    else {
      std::string st("CommitFAIL_COMMIT");
      st += std::to_string(app_context->node_id);
      st += "_";
      st += buf;

      auto msg_size = st.size() + 1;

      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
      
      
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }
#endif

    app_context->rpc->enqueue_response(req_handle, &_resp);
    local_txns->erase(id);
    return;
  }
  std::cerr << __PRETTY_FUNCTION__ << ": Transaction w/ id " << buf << " does not exist ..\n";
  exit(-1);
  print_local_txns();
  assert(false && "Commit:Transaction does not exist");
}


// invoked when receiving a message of kReqTxnPrepare
void req_handler_txnPrepare(erpc::ReqHandle *req_handle, void *context) {
  auto* app_context = static_cast<AppContext *>(context);
  auto& _resp = req_handle->dyn_resp_msgbuf;
  
  bool ack;

#ifdef ENCRYPTION 
// #if 0
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get(), 16);
  char* buf = _buf.get();
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::unique_ptr<char[]> _buf = safe_memcpy(msg, 16);
  char* buf = _buf.get();
#endif
  if (transaction_exists(buf)) {
    std::string id(buf);

    LocalTxn* txn = get_transaction(id);
    int coordinatorId = getCoordinatorId(buf);
    ack = txn->prepareLocalTxn(app_context->log, app_context->RID, coordinatorId, getTxnId(buf)); //FIXME!
    ack = true;

#ifdef ENCRYPTION
    if (ack) {
      std::string st("PrepareACK_ENCRYPTION");
      st += std::to_string(app_context->node_id);
      st += '\0';
      auto msg_size = st.size();

      if (msg_size >= 800) {
        _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }
    else {
      std::string st("PrepareFAIL_ENCRYPTION");
      st += std::to_string(app_context->node_id);
      st += '\0';
      auto msg_size = st.size();

      if (msg_size >= 800) {
          _resp = app_context->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
      bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    }
#else
    if (ack) {
      std::string st("PrepareACK");
      st += std::to_string(app_context->node_id);
      auto msg_size = st.size() + 1;

      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
      
      
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }
    else {
      std::string st("PrepareFAIL");
      st += std::to_string(app_context->node_id);

    
      auto msg_size = st.size() + 1;
      if (msg_size >= 968) {
        _resp = app_context->rpc->alloc_msg_buffer(msg_size);
      }
      else 
        _resp = req_handle->pre_resp_msgbuf;

      app_context->rpc->resize_msg_buffer(&_resp, msg_size);
    
    
      ::memcpy(_resp.buf, st.c_str(), st.size());
      _resp.buf[st.size()] = '\0';
    }
#endif

#ifdef ROLLBACK_PROTECTION
    boost::this_fiber::sleep_for(std::chrono::milliseconds(2));
#endif
    app_context->rpc->enqueue_response(req_handle, &_resp);
    app_context->rpc->run_event_loop_once();

    return;
  }
  exit(-1);
  assert(false && "Prepare:Transaction does not exist");
}

// invoked when receiving a message of kReqTxnBegin
void req_handler_txnBegin(erpc::ReqHandle *req_handle, void *context) {
  /**
   * Each participant will create a local transaction the first
   * time it will receive a Put() or Get() request. The verification
   * of whether the correct number of operations has been executed
   * will be done in the Prepare() phase.
   */
  exit(-1);
  assert(false);
}

void create_local_txn(char* buf) {
  std::string id(buf);
  std::unique_ptr<LocalTxn> new_txn = std::make_unique<LocalTxn>(std::atoi(id.c_str()));
  bool ack = new_txn.get()->beginLocalTxn();
  assert(ack);
  local_txns->insert(id, std::move(new_txn));
}

void req_handler_txnDelete(erpc::ReqHandle *req_handle, void *context) {
  auto* _c = static_cast<AppContext *>(context);
  auto & _resp = req_handle->dyn_resp_msgbuf;

#ifdef ENCRYPTION 
// #if 0
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::string key = extractKey(reinterpret_cast<char*>(dec_msg.get()));
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get()+1, 16);
  char* buf = _buf.get();
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::string key = extractKey(msg);

  std::unique_ptr<char[]> _buf = safe_memcpy(msg + 1, 16);
  char* buf = _buf.get();
#endif

  if (!transaction_exists(buf)) {
    create_local_txn(buf);
  }

  std::string id(buf);
  LocalTxn *txn = get_transaction(id);
  assert(txn && "txn is nullptr");
  bool ack = txn->deleteLocalTxn(key);

#ifdef ENCRYPTION
  if (ack) {
    std::string st("DeleteACK");
    st += id;
    st += '\0';
    auto msg_size = st.size();
    auto enc_msg_size = sizeof_encrypted_buffer(msg_size);
    if (enc_msg_size >= 800) {
    	_resp = _c->rpc->alloc_msg_buffer(enc_msg_size);
    }
    else 
	    _resp = req_handle->pre_resp_msgbuf;
    
    _c->rpc->resize_msg_buffer(&_resp, enc_msg_size);
    txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
  }
  else {
    std::string st("DeleteFAIL");
    st += id;
    st += '\0';
    auto msg_size = st.size();
    auto enc_msg_size = sizeof_encrypted_buffer(msg_size);
    if (enc_msg_size >= 800) {
    	_resp = _c->rpc->alloc_msg_buffer(enc_msg_size);
    }
    else 
	    _resp = req_handle->pre_resp_msgbuf;
    
    _c->rpc->resize_msg_buffer(&_resp, enc_msg_size);
    txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
  }
#else
  if (ack) {
    std::string st("DeleteACK");
    st += id;
    auto msg_size = st.size() + 1;
    if (msg_size >= 968) {
    	_resp = _c->rpc->alloc_msg_buffer(msg_size);
    }
    else 
	    _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[st.size()] = '\0';
  }
  else {
    std::string st("DeleteFAIL");
    st += id;
    auto msg_size = st.size() + 1;
    if (msg_size >= 968) {
    	_resp = _c->rpc->alloc_msg_buffer(msg_size);
    }
    else 
	    _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[st.size()] = '\0';
  }
#endif
  _c->rpc->enqueue_response(req_handle, &_resp);
  _c->rpc->run_event_loop_once();
}


// invoked when receiving a message of kReqTxnPut
void req_handler_txnPut(erpc::ReqHandle *req_handle, void *context) {
  auto & _resp = req_handle->dyn_resp_msgbuf;
#ifdef ENCRYPTION
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::vector<std::string> vec = splitKVPair(reinterpret_cast<char*>(dec_msg.get()));
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get()+1, 16);
  char* buf = _buf.get();
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::vector<std::string> vec = splitKVPair(msg);

  std::unique_ptr<char[]> _buf = safe_memcpy(msg + 1, 16);
  char* buf = _buf.get();
#endif
  if (!transaction_exists(buf)) {
    create_local_txn(buf);
  }

  std::string id(buf);
  LocalTxn *txn = get_transaction(id);
  if (!txn) {
    std::cerr << __PRETTY_FUNCTION__ << ": txn is nullptr\n";
    exit(-1);
  }

  bool ack = txn->putLocalTxn(vec.front(), vec.back());
  auto* _c = static_cast<AppContext *>(context);
  

#ifdef ENCRYPTION
  if (ack) {
    std::string st("ENCRYPTION_PUTACK");
    st += std::to_string(_c->node_id);
    st += id;

    auto msg_size = st.size() + 1;
    st += '\0';

    if (msg_size >= 800) {
      _resp = _c->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
    }
    else 
      _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
    
    bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
    /*
    // @dimitra: check that encryption is okay
    assert(ok);
    auto _msg_size = _resp.get_data_size();
    size_t enc_size = sizeof_encrypted_buffer(_msg_size); 

    auto ptr = safe_decryption(reinterpret_cast<char*>(_resp.buf), enc_size);
    if (::memcmp(const_cast<char*>(st.c_str()), ptr.get(), msg_size) != 0) {
	    std::cerr << "::memcmp failed\n";
	    exit(-2);
    }
    */
  }
  else {
    std::string st("ENCRYPTION_PUTFAIL");
    st += std::to_string(_c->node_id);
    st += id;
    st += '\0';

    auto msg_size = st.size() + 1;
    if (msg_size >= 800) {
      _resp = _c->rpc->alloc_msg_buffer(sizeof_encrypted_buffer(msg_size));
    }
    else 
      _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, sizeof_encrypted_buffer(msg_size));
    bool ok = txn_cipher->encrypt(_resp.buf, const_cast<char*>(st.c_str()), msg_size);
  }

#else
  if (ack) {
    std::string st("PutACK");
    st += id;
    auto msg_size = st.size() + 1;

    if (msg_size >= 968) {
      _resp = _c->rpc->alloc_msg_buffer(msg_size);
    }
    else 
      _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, msg_size);
    
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[st.size()] = '\0';
  }
  else {
    std::string st("PutFAIL");
    auto msg_size = st.size() + 1;
    
    if (msg_size >= 968) {
      _resp = _c->rpc->alloc_msg_buffer(msg_size);
    }
    else 
      _resp = req_handle->pre_resp_msgbuf;

    _c->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[st.size()] = '\0';
  }
#endif
  _c->rpc->enqueue_response(req_handle, &_resp);
  // std::cout << "received and sent response for put for " << id <<  " (data size: " << _resp.get_data_size() << ") " <<  _resp.buf << " " << "\n";
  _c->rpc->run_event_loop_once();
}

// invoked when receiving a message of kReqTxnRead
void req_handler_txnReadForUpdate(erpc::ReqHandle *req_handle, void *context) {
  auto* app_context = static_cast<AppContext *>(context);

#ifdef ENCRYPTION
//#if 0
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get() + 1, 16);
  char* buf = _buf.get();
  char* msg = reinterpret_cast<char*>(dec_msg.get());
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::unique_ptr<char[]> _buf = safe_memcpy(msg + 1, 16);
  char* buf = _buf.get();
#endif
  if (!transaction_exists(buf)) {
    create_local_txn(buf);
  }

  std::string id(buf), value;
  std::string key = getKeyTobeRead(msg);
  LocalTxn *txn = get_transaction(id);
  assert(txn && "txn is nullptr");
  int ack = txn->readForUpdateLocalTxn(key, &value);

#ifdef ENCRYPTION
//#if 0
  if (ack != -1) {
    std::string st("ReadACK");
    auto msg_size = st.size() + 4 + value.size() + 1;
    auto enc_size = sizeof_encrypted_buffer(msg_size);
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (enc_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(enc_size);
    }
    else {
      _resp = req_handle->pre_resp_msgbuf;
    }
    app_context->rpc->resize_msg_buffer(&_resp, enc_size);
    std::unique_ptr<char []> temp_buf = std::make_unique<char[]>(msg_size);
    ::memcpy(temp_buf.get(), st.c_str(), st.size());

    convertIntToByteArray(reinterpret_cast<char*>(temp_buf.get())+st.size(), value.size());

    ::memcpy(temp_buf.get()+ st.size() + 4, value.c_str(), value.size());
    temp_buf.get()[msg_size - 1] = '\0';
    bool ok = txn_cipher->encrypt(_resp.buf, temp_buf.get(), msg_size);
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
  else {
    std::string st("ReadFAIL");
    st += id;
    auto msg_size = st.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    auto enc_msg_size = sizeof_encrypted_buffer(msg_size);
    if (enc_msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(enc_msg_size);
    }
    else
      _resp = req_handle->pre_resp_msgbuf;

    app_context->rpc->resize_msg_buffer(&_resp, enc_msg_size);
    std::unique_ptr<char []> temp_buf = std::make_unique<char[]>(msg_size);
    ::memcpy(temp_buf.get(), st.c_str(), st.size());
    temp_buf.get()[msg_size - 1] = '\0';
    bool ok = txn_cipher->encrypt(_resp.buf, temp_buf.get(), msg_size);
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
#else
  if (ack != -1) {
    std::string st("ReadACK");
    auto msg_size = st.size() + 4 + value.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(msg_size);
    }
    else {
      _resp = req_handle->pre_resp_msgbuf;
    }

    app_context->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    convertIntToByteArray(reinterpret_cast<char*>(_resp.buf)+st.size(), value.size());
    ::memcpy(_resp.buf+ st.size() + 4, value.c_str(), value.size());
    _resp.buf[msg_size - 1] = '\0';
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
  else {
    std::string st("ReadForUPDATEFAIL");
    st += id;
    auto msg_size = st.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(msg_size);
    }
    else {
      _resp = req_handle->pre_resp_msgbuf;
    }
   app_context->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[msg_size - 1] = '\0';
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
#endif
  app_context->rpc->run_event_loop_once();
}

// invoked when receiving a message of kReqTxnRead
void req_handler_txnRead(erpc::ReqHandle *req_handle, void *context) {
  auto* app_context = static_cast<AppContext *>(context);

#ifdef ENCRYPTION
// #if 0
  char* enc_data = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  size_t enc_size = req_handle->get_req_msgbuf()->get_data_size();
  std::unique_ptr<uint8_t[]> dec_msg = safe_decryption(enc_data, enc_size);
  std::unique_ptr<char[]> _buf = safe_memcpy(dec_msg.get() + 1, 16);
  char* buf = _buf.get();
  char* msg = reinterpret_cast<char*>(dec_msg.get());
#else
  char* msg = reinterpret_cast<char*>(req_handle->get_req_msgbuf()->buf);
  std::unique_ptr<char[]> _buf = safe_memcpy(msg + 1, 16);
  char* buf = _buf.get();
#endif
  if (!transaction_exists(buf)) {
    create_local_txn(buf);
  }

  std::string id(buf), value;
  std::string key = getKeyTobeRead(msg);
  LocalTxn *txn = get_transaction(id);
  assert(txn && "txn is nullptr");
  int ack = txn->readLocalTxn(key, &value);

#ifdef ENCRYPTION
// #if 0
  if (ack != -1) {
    std::string st("ReadACK");
    auto msg_size = st.size() + 4 + value.size() + 1;
    auto enc_msg_size = sizeof_encrypted_buffer(msg_size);
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (enc_msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(enc_msg_size);
    }
    else
      _resp = req_handle->pre_resp_msgbuf;

    app_context->rpc->resize_msg_buffer(&_resp, enc_msg_size);
    std::unique_ptr<char []> temp_buf = std::make_unique<char[]>(msg_size);
    ::memcpy(temp_buf.get(), st.c_str(), st.size());
    convertIntToByteArray(reinterpret_cast<char*>(temp_buf.get()) + st.size(), value.size());
    ::memcpy(temp_buf.get() + st.size() + 4, value.c_str(), value.size());
    temp_buf.get()[msg_size - 1] = '\0';
   
    bool ok = txn_cipher->encrypt(_resp.buf, temp_buf.get(), msg_size);
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
  else {
    std::string st("ReadFAIL");
    st += id;
    auto msg_size = st.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    auto enc_msg_size = sizeof_encrypted_buffer(msg_size);
    if (enc_msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(enc_msg_size);
    }
    else
      _resp = req_handle->pre_resp_msgbuf;

    app_context->rpc->resize_msg_buffer(&_resp, enc_msg_size);
    std::unique_ptr<char []> temp_buf = std::make_unique<char[]>(msg_size);
    ::memcpy(temp_buf.get(), st.c_str(), st.size());
    temp_buf.get()[msg_size - 1] = '\0';
    bool ok = txn_cipher->encrypt(_resp.buf, temp_buf.get(), msg_size);

    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
#else
  if (ack != -1) {
    std::string st("ReadACK");
    auto msg_size = st.size() + 4 + value.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(msg_size);
    }
    else
      _resp = req_handle->pre_resp_msgbuf;

    app_context->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    convertIntToByteArray(reinterpret_cast<char*>(_resp.buf)+st.size(), value.size());
    ::memcpy(_resp.buf+ st.size() + 4, value.c_str(), value.size());
    _resp.buf[msg_size - 1] = '\0';
    app_context->rpc->enqueue_response(req_handle, &_resp);
  	// std::cout << "received and sent response for read for " << id <<  " (data size: " << _resp.get_data_size() << ") " <<  _resp.buf << " " << "\n";
  }
  else {
    std::string st("ReadFAIL");
    st += id;
    auto msg_size = st.size() + 1;
    auto & _resp = req_handle->dyn_resp_msgbuf;
    if (msg_size >= 800) {
      _resp = app_context->rpc->alloc_msg_buffer(kMsgSize);
    }
    else
      _resp = req_handle->pre_resp_msgbuf;

    app_context->rpc->resize_msg_buffer(&_resp, msg_size);
    ::memcpy(_resp.buf, st.c_str(), st.size());
    _resp.buf[msg_size - 1] = '\0';
    app_context->rpc->enqueue_response(req_handle, &_resp);
  }
#endif
  app_context->rpc->run_event_loop_once();
}



// @dimitra: these handlers are not used
void req_handler_recoverParticipant(erpc::ReqHandle* req_handle, void* context) {
  std::cout << __PRETTY_FUNCTION__ << " AFKLAJSFLIUASDFODJFLDJFLKJ\n\n\n";
  std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";
  auto& _resp = req_handle->pre_resp_msgbuf; // buf for the reply
  std::cout << __PRETTY_FUNCTION__ << " " << req_handle->get_req_msgbuf()->buf << "\n";
  auto* _c = static_cast<AppContext*>(context);
  _c->rpc->resize_msg_buffer(&_resp, kMsgSize);
  sprintf(reinterpret_cast<char *>(_resp.buf), "ackrecover");

  _c->rpc->enqueue_response(req_handle, &_resp);
  _c->rpc->run_event_loop(2000);
  std::cout << __PRETTY_FUNCTION__ << " Response enqueued\n\n\n";

  // should search and reply back about the luck of the txn.
  participant_recovered = 1;

}

void req_handler_recoverCoordinator(erpc::ReqHandle* req_handle, void* context) {
  // maybe not used (the coordinator that recovers will read the log and retry all questionable transactions)
  std::cout << __FILE__ << " " << __LINE__ << " " << __PRETTY_FUNCTION__ << " " << req_handle->get_req_msgbuf()->buf << "\n";
  if (::memcmp( req_handle->get_req_msgbuf()->buf, "OK", 2) == 0) {
    std::cout << __FILE__ << " " << __LINE__ << " " << __PRETTY_FUNCTION__ << " =============================== \n";
    established_state = 1;
  }
  auto& _resp = req_handle->pre_resp_msgbuf; // buf for the reply
  auto* _c = static_cast<AppContext*>(context);
  _c->rpc->resize_msg_buffer(&_resp, kMsgSize);
  sprintf(reinterpret_cast<char *>(_resp.buf), "AckRecov%d", _c->node_id);
  // LOG_DEBUG_HANDLERS_MSG(__FILE__, __LINE__, __PRETTY_FUNCTION__, req_handle->get_req_msgbuf()->buf);
  _c->rpc->enqueue_response(req_handle, &_resp);
  _c->rpc->run_event_loop(2000);

  // should search and reply back about the luck of the txn.
}

void req_handler_terminate(erpc::ReqHandle*, void*) {
  std::cout << __PRETTY_FUNCTION__ << "\n";
  // ctrl_c_pressed.store(true, std::memory_order_seq_cst);
  // kill(getpid(), SIGINT);
}
