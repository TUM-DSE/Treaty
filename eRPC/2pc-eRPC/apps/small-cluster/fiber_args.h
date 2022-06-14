#pragma once
#include <vector>
#include <map>
#include <boost/fiber/all.hpp>

#include "lib/app_context.h"

#ifdef SCONE
#include "server_app_scone/util.h"
#else
#include "server_app/util.h"
#endif

struct boost_fiber_args {
	std::vector<util::socket_fd> l_sockets;
	std::map<util::socket_fd, util::socket_fd> s_sockets;
    // pointer to the database
	rocksdb::TransactionDB* db = nullptr;           

	int index = -1;
	int fiber_id = -1;
	AppContext* context;
};

struct threadArgs_boost_fibers {
    // sockets which listens for requests
	std::vector<util::socket_fd> listening_sockets;
    // socket which replies back to client
	std::map<util::socket_fd, util::socket_fd> sending_sockets;             
    // pointer to the database
	rocksdb::TransactionDB* db = nullptr; 
	boost::fibers::mutex mtx;
	boost::fibers::condition_variable cv;
	std::string remote_ip;

};