#include "args_parser.h"

int main(int args, char* argv[]) {

	args_parser::ArgumentParser args_p("speicherDB");
	args_p.help_message();
	args_p.parse_input(args, argv);

	return 0;
}
