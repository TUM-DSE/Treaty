cmd_fips_validation_cmac.o = gcc -Wp,-MD,./.fips_validation_cmac.o.d.tmp  -m64 -pthread -I/home/dimitra/eRPC/dpdk/lib/librte_eal/linux/eal/include  -march=corei7 -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2  -I/home/dimitra/eRPC/dpdk/examples/fips_validation/x86_64-native-linuxapp-gcc/include -I/home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include -include /home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -D_GNU_SOURCE -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Werror -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member -DALLOW_EXPERIMENTAL_API -I/home/dimitra/eRPC/dpdk/examples/fips_validation -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Werror -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member    -o fips_validation_cmac.o -c /home/dimitra/eRPC/dpdk/examples/fips_validation/fips_validation_cmac.c 
