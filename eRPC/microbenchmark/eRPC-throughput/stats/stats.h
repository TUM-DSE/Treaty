#pragma once
#include <cstddef>
#include <cstdint>
#include <sys/time.h>

unsigned long get_time();

inline uint64_t return_tsc() { //should be inlined to remove function call overhead
	uint32_t eax;
	uint32_t edx;
	asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
	return (static_cast<uint64_t>(edx) << 32) | eax;
}

uint64_t get_time_in_ms(size_t tsc_value);

long double get_time_in_s(size_t tsc_value);

uint64_t get_time(size_t cpu_cycles);

inline long int get_time_in_ms() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	unsigned long ret = tv.tv_usec/1000;
	ret += (tv.tv_sec * 1000);
	return ret;
}

long double get_rdtsc_freq();
