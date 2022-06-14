#pragma once
#include <stdio.h>
#include "rpc.h"

static const std::string kServerHostname1 	= "129.215.165.58";
static const std::string kServerHostname2 	= "129.215.165.57";

static constexpr uint16_t kUDPPort 		= 31850;
static constexpr uint8_t kReqType 		= 2;

static const size_t RESPONSE_SIZE      		= 64;

static size_t kMsgSize;
