cmd_fm10k_tlv.o = gcc -Wp,-MD,./.fm10k_tlv.o.d.tmp  -m64 -pthread -I/home/dimitra/eRPC/dpdk/lib/librte_eal/linux/eal/include  -march=corei7 -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -I/home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include -include /home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -D_GNU_SOURCE -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member -DALLOW_EXPERIMENTAL_API -Wno-deprecated -Wno-unused-parameter -Wno-unused-value -Wno-strict-aliasing -Wno-format-extra-args -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-but-set-variable -Wno-implicit-fallthrough  -D__USE_MISC -D_GNU_SOURCE -Wno-cast-qual -Wno-error -Wno-int-conversion -Wno-unused-parameter -Wno-strict-prototypes -Wno-old-style-definition -Wno-implicit-function-declaration -Wno-nested-externs -Wno-maybe-uninitialized -Wno-unused-function -Wno-array-bounds -Wno-stringop-overflow -Wno-pointer-to-int-cast -I/home/dimitra/eRPC/build/root/include -I/home/dimitra/eRPC/build/root/include/sys -fPIC -o fm10k_tlv.o -c /home/dimitra/eRPC/dpdk/drivers/net/fm10k/base/fm10k_tlv.c 
