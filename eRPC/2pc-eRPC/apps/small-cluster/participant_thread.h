#pragma once
#include <vector>
#include <map>
#include <boost/fiber/all.hpp>

#include "lib/app_context.h"
#include "common_conf.h"
#include "fiber_args.h"

#ifdef SCONE
#include "server_app_scone/util.h"
#else
#include "server_app/util.h"
#endif

static void participant_fiber(boost_fiber_args* _ptr) {
	while (true) {
		_ptr->context->rpc->run_event_loop_once();
		boost::this_fiber::yield();
		std::this_thread::yield();
	}
}

void participant(int* ptr, erpc::Nexus* nexus, AppContext* context) {

	context->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(context), context->RID, sm_handler);

	context->rpc->retry_connect_on_invalid_rpc_id = true;
	
	/*
    context->node_id = 0;
    connection_t _tmp;
	context->cluster_size = 2;
	context->cluster_map[0] = _tmp;
    */
	std::cout << __PRETTY_FUNCTION__ << " " << std::this_thread::get_id() << "\n";


	std::vector<boost::fibers::fiber> fibers;
	std::vector<boost_fiber_args*> fiber_args;

	for (int i = 0; i < context->nb_fibers; i++) {
		auto ptr = new boost_fiber_args();
		ptr->context = context;
		ptr->fiber_id = (i+1);
		fiber_args.push_back(ptr);
	}

	for (int i = 0; i < context->nb_fibers; i++) {
		fibers.emplace_back(boost::fibers::launch::post, participant_fiber, fiber_args[i]);
	}


	std::cout << "fibers launched .. good luck!\n";

	for (auto& fiber : fibers)
		fiber.join();

	std::cout << "all fibers joined ..\n";
	delete context->rpc;
	return;
	}
