#include <thread>
#include <atomic>
#include <cstddef>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "common_conf.h"
#include "context.h"
#include "util/numautils.h"

#include "args_parser/args_parser.h"

#include "stats/stats.h"
#include "util/util.h"


const int TIMEOUT 		= 10000; // expires after 10000 ms of non activity
const int MAX_THREADS		= 16;

int kMessagesNum, kThreadsNum;

std::string server_uri;
std::string client_uri;

std::string msg_data; 			// represent the random byte stream that should be sent accross network

std::atomic<uint64_t> init;
std::atomic<uint64_t> thread_id;

long int times[MAX_THREADS];

void cont_func(void *, void *tag) { 
	uint64_t offset = 1;
	auto _c = static_cast<context*>(tag);
	delete _c;
}

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

long int start;

static void _thread_run(erpc::Nexus& nexus) {
	uint64_t offset = 1;
	uint64_t id = std::atomic_fetch_add(&thread_id, offset);

	rpc_context _c;
	_c.rpc = new erpc::Rpc<erpc::CTransport>(&nexus, static_cast<void*>(&_c), id, sm_handler);
	_c.rpc->retry_connect_on_invalid_rpc_id = true;

	int session_num = _c.rpc->create_session(client_uri, id);

	while (!_c.rpc->is_connected(session_num)) { 
		_c.rpc->run_event_loop_once();
	}

	fprintf(stdout, "[%s] Connection to %s succeeded\n", server_uri.c_str(), client_uri.c_str());
	usleep(100);


	context* ptr;
	_c.start = get_time_in_ms();
	_c.end = get_time_in_ms();
	while (true) {
		ptr = new context(kMsgSize, _c.rpc, 0, &_c);
		while ((ptr->req.buf == nullptr) || (ptr->resp.buf == nullptr)) {
			_c.rpc->run_event_loop(20); // probably no space left so process some responses first
			ptr = new context(kMsgSize, _c.rpc, 0, &_c);
		}

		erpc::MsgBuffer* req = ptr->get_req();
		erpc::MsgBuffer* resp = ptr->get_resp();

		// ::memcpy(req->buf, msg_data.c_str(), kMsgSize);

		_c.rpc->enqueue_request(session_num, kReqType, req, resp, cont_func, ptr);
		_c.rpc->run_event_loop_once();

	}
	_c.rpc->run_event_loop(10000);

	delete _c.rpc;

}

int main(int args, char* argv[]) {
	args_parser::ArgumentParser args_p("eRPC");
	args_p.parse_input(args, argv);

	kMessagesNum = args_p.operations;
	kThreadsNum = args_p.threads;
	kMsgSize = args_p.value_size;

	assert(kThreadsNum <= MAX_THREADS);

	std::vector<std::thread> threads(kThreadsNum);
	server_uri = kServerHostname1 + ":" + std::to_string(kUDPPort);
	client_uri = kServerHostname2 + ":" + std::to_string(kUDPPort);

	msg_data = randomKey(kMsgSize);

	erpc::Nexus nexus(server_uri, 0, 0); // port 0, numa node 0

	for (int i = 0; i < kThreadsNum; i++) {
		threads[i] = std::thread(_thread_run, std::ref(nexus));
		erpc::bind_to_core(threads[i], 0, i);
	}

	for (auto& t : threads) 
		t.join();

	long int max_time = find_max_time(times, kThreadsNum);
	fprintf(stdout, "Execution Time=%lu\n", max_time);


	for (int i = 0; i < kThreadsNum; i++) 
		fprintf(stdout, "times[%d] = %lu\n", i, times[i]);

	return 0;
}

