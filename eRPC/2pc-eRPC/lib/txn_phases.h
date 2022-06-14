/**
 * The phases of a transaction in the coordinators 
 * and participants side in the 2PC.
 */

#pragma once 
#include <iostream>
#include <string>

enum TWO_PC_PHASES {
	PRE_PREPARE = 0,	/* coordinator's related phase */
	POST_PREPARE,		/* coordinator's related phase */
	PARTICIPANTS_PREPARE,	/* participants */
	COMMIT			/* both */

};

const std::string& decode_txn_phase(int);
