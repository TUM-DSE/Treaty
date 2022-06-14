#include "txn_phases.h"
#include <vector>

using phase_id = int;

static std::vector<std::string> vec = {
	"PRE_PREPARE", 
	"POST_PREPARE", 
	"PARTICIPANTS_PREPARE", 
	"COMMIT"
};

const std::string& decode_txn_phase(phase_id i) {
	return vec[i];
}
