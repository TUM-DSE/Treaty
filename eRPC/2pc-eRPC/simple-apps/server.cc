#include "common.h"
erpc::Rpc<erpc::CTransport> *rpc;
erpc::MsgBuffer req;
erpc::MsgBuffer resp;

void req_handler(erpc::ReqHandle *req_handle, void *) {
	std::cout << __PRETTY_FUNCTION__ << " " << req_handle->get_req_msgbuf()->buf << "\n";
	auto &resp = req_handle->pre_resp_msgbuf;
	rpc->resize_msg_buffer(&resp, kMsgSize);
	sprintf(reinterpret_cast<char *>(resp.buf), "Hello From Server");

	rpc->enqueue_response(req_handle, &resp);
}


void cont_func(void *, void *) { std::cout << __PRETTY_FUNCTION__; printf("%s\n", resp.buf); }

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

int main() {

	std::string server_uri = kServerHostname1 + ":" + std::to_string(kUDPPort);
	erpc::Nexus nexus(server_uri, 0, 0);
	nexus.register_req_func(kReqType, req_handler);
	{


		rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, sm_handler);
		rpc->retry_connect_on_invalid_rpc_id = true;

		std::string client_uri = kClientHostname + ":" + std::to_string(kUDPPort);

		int session_num = rpc->create_session(client_uri, 1);

		while (!rpc->is_connected(session_num)) rpc->run_event_loop_once();

		std::cout << " ... !!! \n";


		req = rpc->alloc_msg_buffer_or_die(kMsgSize);
		resp = rpc->alloc_msg_buffer_or_die(kMsgSize);


		// sprintf(reinterpret_cast<char *>(req.buf), "serverToClient");

		rpc->enqueue_request(session_num, kReqType, &req, &resp, cont_func, nullptr);


		rpc->run_event_loop(10000);

		delete rpc;
	}
}

