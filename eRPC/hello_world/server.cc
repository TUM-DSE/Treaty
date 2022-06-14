#include "common.h"
erpc::Rpc<erpc::CTransport> *rpc;
// erpc::MsgBuffer req;
// erpc::MsgBuffer resp;

void req_handler(erpc::ReqHandle *req_handle, void *) {
	auto &resp = req_handle->pre_resp_msgbuf;
	rpc->resize_msg_buffer(&resp, kMsgSize);
	sprintf(reinterpret_cast<char *>(resp.buf), "hello");
	fprintf(stdout, "resp.buf: %s\n", resp.buf);
	rpc->enqueue_response(req_handle, &resp);
}


// void cont_func(void *, void *) { printf("%s\n", resp.buf); }

int main() {
	std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
	erpc::Nexus nexus(server_uri, 0, 0);
	nexus.register_req_func(kReqType, req_handler);

	rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, nullptr);


	/*
	std::string client_uri = kClientHostname + ":" + std::to_string(kUDPPort);
	int session_num = rpc->create_session(client_uri, 0);

	while (!rpc->is_connected(session_num)) rpc->run_event_loop_once();
	fprintf(stdout, "%s: is connected\n", server_uri);
	*/
	// req = rpc->alloc_msg_buffer_or_die(kMsgSize);
	// resp = rpc->alloc_msg_buffer_or_die(kMsgSize);

	// rpc->enqueue_request(session_num, kReqType, &req, &resp, cont_func, nullptr);

	rpc->run_event_loop(100000);
}
