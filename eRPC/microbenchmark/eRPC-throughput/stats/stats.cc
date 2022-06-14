#include "stats.h"

#include <cstdint>

struct CrystalClockFactors {
	size_t crystal_clock_fq;
	static constexpr size_t k = 1; //This might be different, in theory we should be able to fetch it from MSR_PLATFORM_INFO, but 1 works for me
	size_t ART_fq; //= crystal_clock_fq * k;
	uint32_t denominator; //AKA CPUID.15H.EAX[31:0]
	uint32_t numerator;  //AKA CPUID.15H.EBX[31:0]

	CrystalClockFactors() {
		uint32_t a = 0x15;
		uint32_t c = 0x0;
		asm volatile ("cpuid" : "=a" (denominator), "=b" (numerator), "=c" (crystal_clock_fq) : "a" (a), "c" (c) : "edx");
    if (crystal_clock_fq != 0x0) {
      ART_fq = crystal_clock_fq * k;
      return;
    }
    a = 0x16;
    c = 0x0;
    uint32_t base_freq;
    uint32_t max_freq;
    uint32_t bus_freq;
    asm volatile ("cpuid" : "=a" (base_freq), "=b" (max_freq), "=c" (bus_freq) : "a" (a), "c" (c) : "edx");
    crystal_clock_fq = static_cast<size_t>(base_freq) * 1'000'000 * denominator / numerator;
    ART_fq = crystal_clock_fq * k;
	}
};

static CrystalClockFactors const clock_info;

// Not to be used when benchmarking
unsigned long get_time() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	unsigned long ret = tv.tv_usec;
	ret /= 1000;
	ret += (tv.tv_sec * 1000);
	return ret;
}


template<class T, size_t NUMERATOR, size_t DENUMERATOR>
T get_time_(size_t tsc_value) {
	typedef unsigned int uint128_t __attribute__((mode(TI)));
	auto ART_Value = (static_cast<uint128_t>(tsc_value) * clock_info.denominator) / clock_info.numerator;
	T time = (ART_Value * NUMERATOR) / (static_cast<T>(clock_info.ART_fq) * DENUMERATOR); //HOWEVER this breaks with a big tsc_value
	return time;
}

long double get_time_in_s(size_t tsc_value) {
	return get_time_<long double, 1, 1>(tsc_value);
}

uint64_t get_time_in_ms(size_t tsc_value) {
	return get_time_<uint64_t, 1000, 1>(tsc_value);
}

uint64_t get_time(size_t tsc_value) {
	return get_time_in_ms(tsc_value);
}

long double get_rdtsc_freq() {
	return (clock_info.numerator / (clock_info.denominator * static_cast<long double>(clock_info.ART_fq)));
}
