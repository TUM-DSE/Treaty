/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "rte_cpuflags.h"

#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "rte_cpuid.h"

/**
 * Struct to hold a processor feature entry
 */
struct feature_entry {
	uint32_t leaf;				/**< cpuid leaf */
	uint32_t subleaf;			/**< cpuid subleaf */
	uint32_t reg;				/**< cpuid register */
	uint32_t bit;				/**< cpuid register bit */
#define CPU_FLAG_NAME_MAX_LEN 64
	char name[CPU_FLAG_NAME_MAX_LEN];       /**< String for printing */
};

#define FEAT_DEF(name, leaf, subleaf, reg, bit) \
	[RTE_CPUFLAG_##name] = {leaf, subleaf, reg, bit, #name },

const struct feature_entry rte_cpu_feature_table[] = {
	FEAT_DEF(SSE3, 0x00000001, 0, RTE_REG_ECX,  0)
	FEAT_DEF(PCLMULQDQ, 0x00000001, 0, RTE_REG_ECX,  1)
	FEAT_DEF(DTES64, 0x00000001, 0, RTE_REG_ECX,  2)
	FEAT_DEF(MONITOR, 0x00000001, 0, RTE_REG_ECX,  3)
	FEAT_DEF(DS_CPL, 0x00000001, 0, RTE_REG_ECX,  4)
	FEAT_DEF(VMX, 0x00000001, 0, RTE_REG_ECX,  5)
	FEAT_DEF(SMX, 0x00000001, 0, RTE_REG_ECX,  6)
	FEAT_DEF(EIST, 0x00000001, 0, RTE_REG_ECX,  7)
	FEAT_DEF(TM2, 0x00000001, 0, RTE_REG_ECX,  8)
	FEAT_DEF(SSSE3, 0x00000001, 0, RTE_REG_ECX,  9)
	FEAT_DEF(CNXT_ID, 0x00000001, 0, RTE_REG_ECX, 10)
	FEAT_DEF(FMA, 0x00000001, 0, RTE_REG_ECX, 12)
	FEAT_DEF(CMPXCHG16B, 0x00000001, 0, RTE_REG_ECX, 13)
	FEAT_DEF(XTPR, 0x00000001, 0, RTE_REG_ECX, 14)
	FEAT_DEF(PDCM, 0x00000001, 0, RTE_REG_ECX, 15)
	FEAT_DEF(PCID, 0x00000001, 0, RTE_REG_ECX, 17)
	FEAT_DEF(DCA, 0x00000001, 0, RTE_REG_ECX, 18)
	FEAT_DEF(SSE4_1, 0x00000001, 0, RTE_REG_ECX, 19)
	FEAT_DEF(SSE4_2, 0x00000001, 0, RTE_REG_ECX, 20)
	FEAT_DEF(X2APIC, 0x00000001, 0, RTE_REG_ECX, 21)
	FEAT_DEF(MOVBE, 0x00000001, 0, RTE_REG_ECX, 22)
	FEAT_DEF(POPCNT, 0x00000001, 0, RTE_REG_ECX, 23)
	FEAT_DEF(TSC_DEADLINE, 0x00000001, 0, RTE_REG_ECX, 24)
	FEAT_DEF(AES, 0x00000001, 0, RTE_REG_ECX, 25)
	FEAT_DEF(XSAVE, 0x00000001, 0, RTE_REG_ECX, 26)
	FEAT_DEF(OSXSAVE, 0x00000001, 0, RTE_REG_ECX, 27)
	FEAT_DEF(AVX, 0x00000001, 0, RTE_REG_ECX, 28)
	FEAT_DEF(F16C, 0x00000001, 0, RTE_REG_ECX, 29)
	FEAT_DEF(RDRAND, 0x00000001, 0, RTE_REG_ECX, 30)
	FEAT_DEF(HYPERVISOR, 0x00000001, 0, RTE_REG_ECX, 31)

	FEAT_DEF(FPU, 0x00000001, 0, RTE_REG_EDX,  0)
	FEAT_DEF(VME, 0x00000001, 0, RTE_REG_EDX,  1)
	FEAT_DEF(DE, 0x00000001, 0, RTE_REG_EDX,  2)
	FEAT_DEF(PSE, 0x00000001, 0, RTE_REG_EDX,  3)
	FEAT_DEF(TSC, 0x00000001, 0, RTE_REG_EDX,  4)
	FEAT_DEF(MSR, 0x00000001, 0, RTE_REG_EDX,  5)
	FEAT_DEF(PAE, 0x00000001, 0, RTE_REG_EDX,  6)
	FEAT_DEF(MCE, 0x00000001, 0, RTE_REG_EDX,  7)
	FEAT_DEF(CX8, 0x00000001, 0, RTE_REG_EDX,  8)
	FEAT_DEF(APIC, 0x00000001, 0, RTE_REG_EDX,  9)
	FEAT_DEF(SEP, 0x00000001, 0, RTE_REG_EDX, 11)
	FEAT_DEF(MTRR, 0x00000001, 0, RTE_REG_EDX, 12)
	FEAT_DEF(PGE, 0x00000001, 0, RTE_REG_EDX, 13)
	FEAT_DEF(MCA, 0x00000001, 0, RTE_REG_EDX, 14)
	FEAT_DEF(CMOV, 0x00000001, 0, RTE_REG_EDX, 15)
	FEAT_DEF(PAT, 0x00000001, 0, RTE_REG_EDX, 16)
	FEAT_DEF(PSE36, 0x00000001, 0, RTE_REG_EDX, 17)
	FEAT_DEF(PSN, 0x00000001, 0, RTE_REG_EDX, 18)
	FEAT_DEF(CLFSH, 0x00000001, 0, RTE_REG_EDX, 19)
	FEAT_DEF(DS, 0x00000001, 0, RTE_REG_EDX, 21)
	FEAT_DEF(ACPI, 0x00000001, 0, RTE_REG_EDX, 22)
	FEAT_DEF(MMX, 0x00000001, 0, RTE_REG_EDX, 23)
	FEAT_DEF(FXSR, 0x00000001, 0, RTE_REG_EDX, 24)
	FEAT_DEF(SSE, 0x00000001, 0, RTE_REG_EDX, 25)
	FEAT_DEF(SSE2, 0x00000001, 0, RTE_REG_EDX, 26)
	FEAT_DEF(SS, 0x00000001, 0, RTE_REG_EDX, 27)
	FEAT_DEF(HTT, 0x00000001, 0, RTE_REG_EDX, 28)
	FEAT_DEF(TM, 0x00000001, 0, RTE_REG_EDX, 29)
	FEAT_DEF(PBE, 0x00000001, 0, RTE_REG_EDX, 31)

	FEAT_DEF(DIGTEMP, 0x00000006, 0, RTE_REG_EAX,  0)
	FEAT_DEF(TRBOBST, 0x00000006, 0, RTE_REG_EAX,  1)
	FEAT_DEF(ARAT, 0x00000006, 0, RTE_REG_EAX,  2)
	FEAT_DEF(PLN, 0x00000006, 0, RTE_REG_EAX,  4)
	FEAT_DEF(ECMD, 0x00000006, 0, RTE_REG_EAX,  5)
	FEAT_DEF(PTM, 0x00000006, 0, RTE_REG_EAX,  6)

	FEAT_DEF(MPERF_APERF_MSR, 0x00000006, 0, RTE_REG_ECX,  0)
	FEAT_DEF(ACNT2, 0x00000006, 0, RTE_REG_ECX,  1)
	FEAT_DEF(ENERGY_EFF, 0x00000006, 0, RTE_REG_ECX,  3)

	FEAT_DEF(FSGSBASE, 0x00000007, 0, RTE_REG_EBX,  0)
	FEAT_DEF(BMI1, 0x00000007, 0, RTE_REG_EBX,  2)
	FEAT_DEF(HLE, 0x00000007, 0, RTE_REG_EBX,  4)
	FEAT_DEF(AVX2, 0x00000007, 0, RTE_REG_EBX,  5)
	FEAT_DEF(SMEP, 0x00000007, 0, RTE_REG_EBX,  6)
	FEAT_DEF(BMI2, 0x00000007, 0, RTE_REG_EBX,  7)
	FEAT_DEF(ERMS, 0x00000007, 0, RTE_REG_EBX,  8)
	FEAT_DEF(INVPCID, 0x00000007, 0, RTE_REG_EBX, 10)
	FEAT_DEF(RTM, 0x00000007, 0, RTE_REG_EBX, 11)
	FEAT_DEF(AVX512F, 0x00000007, 0, RTE_REG_EBX, 16)

	FEAT_DEF(LAHF_SAHF, 0x80000001, 0, RTE_REG_ECX,  0)
	FEAT_DEF(LZCNT, 0x80000001, 0, RTE_REG_ECX,  4)

	FEAT_DEF(SYSCALL, 0x80000001, 0, RTE_REG_EDX, 11)
	FEAT_DEF(XD, 0x80000001, 0, RTE_REG_EDX, 20)
	FEAT_DEF(1GB_PG, 0x80000001, 0, RTE_REG_EDX, 26)
	FEAT_DEF(RDTSCP, 0x80000001, 0, RTE_REG_EDX, 27)
	FEAT_DEF(EM64T, 0x80000001, 0, RTE_REG_EDX, 29)

	FEAT_DEF(INVTSC, 0x80000007, 0, RTE_REG_EDX,  8)
};

/*#if defined(SCONE)

#define __get_cpuid_max(a, b) 0x80000000

#if defined(__cpuid_count)
#undef __cpuid_count
#define __cpuid_lookup_err(level, count) \
    do {\
        fprintf(stderr, "Could not find cpuid lookup for level: %u count: %u\n", level, count);\
        exit(1);\
    } while(0)

#define __cpuid_count_value(level, count) \
       ((uint64_t)(level) << 32) | (count)

#define __cpuid_case(level, count, a, _a, b,  _b, c, _c, d, _d) \
        case __cpuid_count_value(level, count): a = _a; b = _b; c = _c; d = _d; break;

#define __cpuid_count(level, count, a, b, c, d) \
        do {\
            switch(__cpuid_count_value(level, count)) {\
                __cpuid_case(1, 0, a, 0x506e3, b, 0x5100800, c, 0x7ffafbff, d, 0xbfebfbff) \
                __cpuid_case(7, 0, a, 0x0, b, 0x29c6fbf, c, 0x0, d, 0x0) \
                default:    __cpuid_lookup_err(level, count);\
            }\
        } while(0)
#endif //__cpuid_count
#endif //SCONE*/

int
rte_cpu_get_flag_enabled(enum rte_cpu_flag_t feature)
{
	const struct feature_entry *feat;
	cpuid_registers_t regs;
	unsigned int maxleaf;

	if (feature >= RTE_CPUFLAG_NUMFLAGS)
		/* Flag does not match anything in the feature tables */
		return -ENOENT;

	feat = &rte_cpu_feature_table[feature];

	if (!feat->leaf)
		/* This entry in the table wasn't filled out! */
		return -EFAULT;

	maxleaf = __wrap__get_cpuid_max(feat->leaf & 0x80000000, NULL);

	if (maxleaf < feat->leaf)
		return 0;

	 __wrap_cpuid_count(feat->leaf, feat->subleaf,
			 regs[RTE_REG_EAX], regs[RTE_REG_EBX],
			 regs[RTE_REG_ECX], regs[RTE_REG_EDX]);

	/* check if the feature is enabled */
	return (regs[feat->reg] >> feat->bit) & 1;
}

const char *
rte_cpu_get_flag_name(enum rte_cpu_flag_t feature)
{
	if (feature >= RTE_CPUFLAG_NUMFLAGS)
		return NULL;
	return rte_cpu_feature_table[feature].name;
}
