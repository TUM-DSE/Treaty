cmd_main.o = gcc -Wp,-MD,./.main.o.d.tmp  -m64 -pthread -I/home/dimitra/eRPC/dpdk/lib/librte_eal/linux/eal/include  -march=corei7 -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2  -I/home/dimitra/eRPC/dpdk/examples/multi_process/hotplug_mp/x86_64-native-linuxapp-gcc/include -I/home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include -include /home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -D_GNU_SOURCE -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Werror -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member    -o main.o -c /home/dimitra/eRPC/dpdk/examples/multi_process/hotplug_mp/main.c 