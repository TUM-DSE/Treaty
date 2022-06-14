/**
 * Information about the cluster. UDPPort to be used and any handlers
 * required for the application-terminatio.
 */

#include "termination.h"
#include "rpc.h"

static const std::string kServerHostname = "129.215.165.54"; 		//donna
static const std::string kClientHostname_1 = "129.215.165.57"; 		//amy: 129.215.165.57
static const std::string kClientHostname_2 = "129.215.165.58"; 		//clara: 129.215.165.58 rose: 129.215.165.52

static constexpr uint16_t kUDPPort = 31850;

void ctrl_c_handler(int) {
	ctrl_c_pressed = true;
	exit(1);
}
