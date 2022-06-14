#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define __SCONE_val(level, count) \
	(((uint64_t)(level) << 32) | (count))

#define __SCONE_cpuid_case(level, count, _a, _b, _c, _d) \
	case __SCONE_val(level, count): *a = _a; *b = _b; *c = _c; *d = _d; break;

static inline void __SCONE_cpuid_err(uint32_t const level, uint32_t const count) {
	fprintf(stderr, "Could not find cpuid lookup for level: %u count: %u\n", level, count);
	exit(1);
}

static inline unsigned int __SCONE_get_cpuid_max(unsigned int const a, unsigned int * const ptr) {
	(void) a;
	if (ptr == NULL) //else look at: /opt/scone/cross-compiler/lib/gcc/x86_64-linux-musl/7.1.0/include/cpuid.h:189
		return 0x80000000;
	fprintf(stderr, "[%s:%d] ptr is not NULL ptr(%p)\n", __FILE__, __LINE__, (void *)ptr);
	exit(1);
	return 0x0;
}

//static inline void __SCONE_cpuid_count(uint

static inline void __SCONE_cpuid_count(uint32_t const level, uint32_t const count, uint32_t * a, uint32_t * b, uint32_t * c, uint32_t * d) {
	switch(__SCONE_val(level, count)) {
		// TODO: CPU-specific
	//	__SCONE_cpuid_case(0x0, 0x0, 0x16, 0x756e6547, 0x6c65746e, 0x49656e69);
	//	__SCONE_cpuid_case(0x1, 0x0, 0x506e3, 0x5100800, 0x7ffafbff, 0xbfebfbff);
	//	__SCONE_cpuid_case(0x7, 0x0, 0x0, 0x29c6fbf, 0x0, 0x0);
	//	__SCONE_cpuid_case(0x15, 0x0, 0x2, 0x12c, 0x0, 0x0);


		__SCONE_cpuid_case(0x0, 0x0, 0x16, 0x756e6547, 0x6c65746e, 0x49656e69);
		__SCONE_cpuid_case(0x1, 0x0, 0x906ec, 0x6100800, 0x7ffafbff, 0xbfebfbff);
		__SCONE_cpuid_case(0x7, 0x0, 0x0, 0x29c6fbf, 0x40000000, 0xbc002400);
		__SCONE_cpuid_case(0x15, 0x0, 0x2, 0x12c, 0x0, 0x0);
		default: 
			__SCONE_cpuid_err(level, count);
	}
}

#define __wrap_cpuid_count(level, count, a, b, c, d) \
	__SCONE_cpuid_count(level, count, &a, &b, &c, &d)

static inline int __wrap_cpuid(uint32_t level, uint32_t * a, uint32_t * b, uint32_t * c, uint32_t * d) {
	__SCONE_cpuid_count(level, 0, a, b, c, d);
	return 1;
}

/* static inline int __wrap_cpuid(uint32_t level, uint32_t & a, uint32_t & b, uint32_t & c, uint32_t & d) {
	return __wrap_cpuid(level, &a, &b, &c, &d);
} */

static inline unsigned int __wrap__get_cpuid_max(unsigned int const a, unsigned int * const ptr) {
	return __SCONE_get_cpuid_max(a, ptr);
}

#define X86_CPUID(type, out) \
	__SCONE_cpuid_count(type, 0, &out[0], &out[1], &out[2], &out[3])

#define X86_CPUID_SUBLEVEL(type, level, out) \
	__SCONE_cpuid_count(type, level, &out[0], &out[1], &out[2], &out[3])
