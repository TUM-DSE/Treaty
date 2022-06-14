#pragma once 
#include <iostream>
#include <getopt.h>                                     /**< getopt_long() */
#include <unistd.h>
#include <cassert>



static const char help_msg[]=
"Description:\n"
"	-h, --help 				help page\n"
" 	-t NUM, --threads=NUM	 		number of threads\n"
"	-o NUM, --operations=NUM		number of messages to be sent\n"
"	-u NUM, --value_size=NUM		size of the message";


namespace args_parser {

	static struct option long_cmd_opts[] = {
		{"help",                no_argument,       0, 'h'},
		{"threads",        	required_argument, 0, 't'},
		{"operations",        	required_argument, 0, 'o'},
		{"value_size",		required_argument, 0, 'u'},
		{0, 0, 0, 0}
	};

	class ArgumentParser {
		public:
			ArgumentParser(std::string program_name) : name(program_name), threads(1), operations(100), value_size(64) {}
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
				std::string ss = name + " starts with :\n" +
				"Threads=" + std::to_string(threads) + "\n" + 
				"Messages Number=" + std::to_string(operations) + "\n" +
				"Message Size=" + std::to_string(value_size) + "\n";
				print_msg(ss);
			}


			void parse_input(int argc, char* argv[])  {
				int cmd, opt_index = 0;;
				while ((cmd = getopt_long(argc, argv,
								":Hhwf:i:m:Tt:l:o:u:e:c:a:p:sdDrvVIPR:",
								args_parser::long_cmd_opts, &opt_index)) != -1) {

					switch (cmd) {
						case 'h':
							help_message();
							return;
						case 't':
							threads = atoi(optarg);
							break;
						case 'o':
							operations = atoi(optarg);
							break;
						case 'u':
							value_size = atoi(optarg);
							break;
						default:
							print_error("Error: Unknown argument\n");
							return;

					}
				}

				to_string();
			}




	//	private:
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
			int threads, operations, value_size;
	};

} // args_parser ends
