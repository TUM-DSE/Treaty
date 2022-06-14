/**
 * Debugging utils.
 */

#pragma once
#include <iostream>
#include <thread>
#include <stdarg.h>

#ifdef SUPER_DEBUGGING
#warning SUPER_DEBUGGING is defined
inline void _SUPER_DEBUGGING (const char* filename, int line, const char* function_name, int num, ...) {
	va_list arguments;                     
	std::string s;

	// Initializing arguments to store all values after num.
	va_start(arguments, num);           

	/** 
	 * We still rely on the function caller to tell us how
	 * many there are.
	 */
	for (int i = 0; i < num; i++) {
		s += std::string(va_arg(arguments, char*));
		s += " ";
	}

	std::cout << filename << " (line " << line <<"): " << function_name << " " << s << "\n";
	va_end(arguments);
}

inline void __print_debug (const char* filename, int line, const char* function_name, ...) {
	va_list arguments;                     
	char *format = NULL;// *format = '%s';
	std::string s;
	va_start(arguments, function_name);
	format = va_arg(arguments, char*);
	while (format != NULL) {
		// s += std::string(format);
		s += " ";
		// vprintf(format, arguments);
		format = va_arg(arguments, char*);
	}
	// std::cout << filename << " (line " << line <<"): " << function_name << " " << s << "\n";
	va_end(arguments);
}
#endif


#define print_debug(...) __print_debug(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);

#ifdef DEBUG_2PC

#define	LOG_DEBUG(fname, lineno, fxname)				//std::cout << fname << ":" << lineno<< "\t" << fxname << "\n";
#define	LOG_DEBUG_MSG(fname, lineno, fxname, msg)			// std::cout << fname << ":" << lineno<< "\t" << fxname << "\t" << msg << "\n";

#define	LOG_DEBUG_HANDLERS(fname, lineno, fxname)			// std::cout <<"[Thread: " << std::this_thread::get_id() << "] "<< fname << ":" << lineno<< "\t" << fxname << "\n";
#define	LOG_DEBUG_HANDLERS_MSG(fname, lineno, fxname, msg)		// std::cout << "[Thread: " << std::this_thread::get_id() << "] " << fname << ":" << lineno<< "\t" << fxname << "\t" << msg << "\n";

#else

#define	LOG_DEBUG(fname, lineno, fxname) 			
#define	LOG_DEBUG_MSG(fname, lineno, fxname, msg)		
#define	LOG_DEBUG_HANDLERS(fname, lineno, fxname)		
#define	LOG_DEBUG_HANDLERS_MSG(fname, lineno, fxname, msg)		

#endif
