#pragma once
#include "rpc.h"
#include "common.h"

class rpc_context {
	public:
		rpc_context() = default;
		~rpc_context() = default;
		erpc::Rpc<erpc::CTransport> *rpc;
		uint64_t served_reqs = 0;
		uint64_t prev_served_reqs = 0; // for calculating deltas
		int start = 0, end = 0;
};


class context {
	public:
		context() = delete;

		using request_id = uint64_t; 
		context(size_t kMsgSize, erpc::Rpc<erpc::CTransport>* _rpc, request_id _id, rpc_context* _rpc_context) 
			: rpc(_rpc), i(_id), rpc_cont(_rpc_context) {
			req = rpc->alloc_msg_buffer(kMsgSize);
			resp = rpc->alloc_msg_buffer(RESPONSE_SIZE);
		}

		~context() {
			rpc->free_msg_buffer(req);
			rpc->free_msg_buffer(resp);
		}

		erpc::MsgBuffer* get_req() {
			return &req;
		}

		erpc::MsgBuffer* get_resp() {
			return &resp;
		}

		erpc::MsgBuffer req;
		erpc::MsgBuffer resp;

		erpc::Rpc<erpc::CTransport>* rpc; // no ownership -- the application will free the ptr

		request_id i; // the id of the request that created this context

		rpc_context* rpc_cont;

};	
