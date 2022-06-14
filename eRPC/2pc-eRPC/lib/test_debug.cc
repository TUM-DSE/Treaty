#define SUPER_DEBUGGING

#include "debug.h"

int main(int args, char* argv[]) {

	_SUPER_DEBUGGING(__FILE__, __LINE__, __PRETTY_FUNCTION__, 3, "Dimitra", "Giantsidi", "END");
	LOL( "Dimitra", "Giantsidi", "END");

	return 0;
}
