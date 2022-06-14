#pragma once 
#include <iostream>
#include <getopt.h>                                     /**< getopt_long() */
#include <unistd.h>
#include <cassert>



static const char help_msg[]=
"Description:\n"
"	-h, --help 				help page\n"
" 	-c NUM, --coordinators=NUM 		number of coordinators in this node\n"
"	-t NUM, --transactions=NUM		number of transactions to be executed\
		";


namespace args_parser {

	static struct option long_cmd_opts[] = {
		{"help",                no_argument,       0, 'h'},
		{"coordinators",        required_argument, 0, 'c'},
		{"transactions",	required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	class ArgumentParser {
		public:
			ArgumentParser(std::string program_name) : name(program_name), c(-1), t(-1) {}
			~ArgumentParser() = default;

			// no copy/move semantics
			ArgumentParser(ArgumentParser const& other) = delete;
			ArgumentParser(ArgumentParser&& other) = delete;
			ArgumentParser& operator = (ArgumentParser const& other) = delete;
			ArgumentParser& operator = (ArgumentParser&& other) = delete;

			void help_message() {
				print_msg(help_msg);
			}

			void to_string() {
				if (t > 0 && c > 0) {
					std::string ss = "Program: " + name + ":\n";
						"\tNumber of Txn Coordinators = " + std::to_string(c) + "\n" + 
						"\tNumber of Transactions = " + std::to_string(t) + "\n";
					print_msg(ss);
					return;
				}

				if (c > 0) {
					std::string ss = "Number of Txn Coordinators = " + std::to_string(c) + "\n";
					return;
				}

				if (t > 0)
					std::string ss = "Number of Transactions = " + std::to_string(t) + "\n";

			}

			void parse_input(int argc, char* argv[])  {
				int cmd, opt_index = 0;;
				int _input_args = 0;
				while ((cmd = getopt_long(argc, argv,
								":Hhwf:i:m:Tt:l:o:u:e:c:a:p:sdDrvVIPR:",
								args_parser::long_cmd_opts, &opt_index)) != -1) {

					switch (cmd) {
						case 'h':
							help_message();
							return;
						case 'c':
							_input_args++;
							c = atoi(optarg);
							break;
						case 't':
							_input_args++;
							t = atoi(optarg);
							break;
						default:
							print_error("Error:Unknown argument\n");
							return;

					}
				}

				if (t == -1)
					t = 1000;

				to_string();
			}



			int get_num_of_txns() {
				return t;
			}

			int get_num_of_coordinators() {
				return c;
			}

		private:
			template <typename T>
				void print_msg(T msg) {
					std::cout << msg << "\n";
				}

			template <typename T>
				void print_error(T msg) {
					std::cerr<< msg << "\n";
					assert(false);
				}

			// argument options values
			std::string name;
			int c, t;
	};

} // args_parser ends
