/**
 * Information about the cluster. UDPPort to be used and any handlers
 * required for the application-termination.
 */

#pragma once
#include "termination.h"
#include "rpc.h"

const std::string kdonnaHostname 	= "129.215.165.54"; 	
const std::string kmarthaHostname 	= "129.215.165.57"; // "129.215.165.53"; 
const std::string kroseHostname 		= "129.215.165.52";

constexpr uint16_t kUDPPort = 31850;

void ctrl_c_handler(int) {
	ctrl_c_pressed = true;
	exit(1);
}

void sm_handler(int local_session, erpc::SmEventType, erpc::SmErrType, void *);
