#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "config.h"
#include <memory>

namespace msg {
	// std::string TxnPutMsg(std::string, std::string, int, int);
	std::unique_ptr<char[]> TxnPutMsg(std::string, std::string, int, int);
	std::unique_ptr<char[]> TxnDeleteMsg(std::string, int, int);

	std::string TxnReadMsg(std::string&, int, int);

	std::string TxnPrepMsg(int, int);

	std::string TxnCommitMsg(int, int);

	std::string TxnBeginMsg(int, int);
	

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	static int stringLength = sizeof(alphanum) - 1;


	static char getRandom();

	std::string randomKey(size_t);
	std::string getVal(size_t);
}
