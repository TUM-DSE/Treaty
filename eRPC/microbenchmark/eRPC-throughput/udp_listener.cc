#include <thread>
#include <atomic>
#include <cstddef>

#include "common_conf.h"
#include "context.h"
#include "util/numautils.h"



#include "args_parser/args_parser.h"
#include "stats/stats.h"
#include "util/util.h"


int kMessagesNum, kThreadsNum;

std::string server_uri;
std::string client_uri;

std::string kMsgData; // represent the random byte stream that should be sent accross network

std::atomic<uint64_t> thread_id;

uint64_t offset = 1;

std::array<rpc_context*, 16> _contexes;

void req_handler(erpc::ReqHandle *req_handle, void *context) {
	auto* _c = static_cast<rpc_context*>(context);
	_c->served_reqs++;


	auto &resp = req_handle->pre_resp_msgbuf;
	if (req_handle->get_req_msgbuf()->get_data_size() != kMsgSize) {
		std::cerr << "Error\n";
	}
	_c->rpc->resize_msg_buffer(&resp, RESPONSE_SIZE);
	_c->rpc->enqueue_response(req_handle, &resp);
}


void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}


static void _thread_run(erpc::Nexus& nexus) {
	uint64_t id = std::atomic_fetch_add(&thread_id, offset);

	rpc_context _c;
	_c.rpc = new erpc::Rpc<erpc::CTransport>(&nexus, static_cast<void*>(&_c), id, sm_handler);
	// _c.rpc->retry_connect_on_invalid_rpc_id = true;

	_contexes[id] = &_c;
	// int session_num = _c.rpc->create_session(client_uri, thread_id);

	fprintf(stdout, "[%s - %d] connection to %s succeeded\n", server_uri.c_str(), id,  client_uri.c_str());

	while (true) {
		// _c.rpc->run_event_loop(1000000);
		_c.rpc->run_event_loop_once();
	}

	delete _c.rpc;

}

int main(int args, char* argv[]) {
	args_parser::ArgumentParser args_p("eRPC listener");
	args_p.parse_input(args, argv);

	kMessagesNum = args_p.operations;
	kThreadsNum = args_p.threads;
	kMsgSize = args_p.value_size;
	kMsgData = randomKey(kMsgSize);

	std::vector<std::thread> threads(kThreadsNum);
	server_uri = kServerHostname2 + ":" + std::to_string(kUDPPort);
	client_uri = kServerHostname1 + ":" + std::to_string(kUDPPort);


	erpc::Nexus nexus(server_uri, 0, 0); // port 0, numa node 0
	nexus.register_req_func(kReqType, req_handler);

	for (int i = 0; i < kThreadsNum; i++) {
		threads[i] = std::thread(_thread_run, std::ref(nexus));
		erpc::bind_to_core(threads[i], 0, i);
	}

	uint64_t _s = return_tsc();
	uint64_t _e = return_tsc();
	_s = get_time_in_ms();
	_e = get_time_in_ms();
	usleep(10000000);
	uint64_t total = 0;
	while (true) {
		if ((_e - _s) > 1000) {
			for (int i = 0; i < kThreadsNum; i++) {
				uint64_t served = _contexes[i]->served_reqs;
				uint64_t prev_served = _contexes[i]->prev_served_reqs;
				fprintf(stdout, "[%d] finished requests: %" PRIu64 " in %lu ms, MBytes = %" PRIu64 ", MBytes/sec = %7.3f\n",\
						   i, (served - prev_served), (_e-_s),\
						   (served - prev_served)*kMsgSize/(1000*1000),\
						(served - prev_served)*kMsgSize*1000.0/(1000.0*1000.0)/((_e-_s)*1.0));
				_contexes[i]->prev_served_reqs = served;
				std::cout << "served : " << served << "\n";
				total += (served - prev_served);
			}
			fprintf(stdout, "[SUM] finished requests: %" PRIu64 " in %lu ms, MBytes = %" PRIu64 ", MBytes/sec = %7.3f, Gbits/se = %7.3f\n",\
						   total, (_e-_s),\
						   total*kMsgSize/(1000*1000),\
						total*kMsgSize*1000.0/(1000.0*1000.0)/((_e-_s)*1.0), total*kMsgSize*1000.0/(1000.0*1000.0)/((_e-_s)*1.0*125.0));
			
			fprintf(stdout, "-------------------------------\n\n");
			total = 0;
			_s = _e;
		}
		sleep(1);
		_e = get_time_in_ms();
	}

	for (auto& t : threads) 
		t.join();

	return 0;
}

