cmd_otx2_dpi_rawdev.o = gcc -Wp,-MD,./.otx2_dpi_rawdev.o.d.tmp  -m64 -pthread -I/home/dimitra/eRPC/dpdk/lib/librte_eal/linux/eal/include  -march=corei7 -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -I/home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include -include /home/dimitra/eRPC/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -D_GNU_SOURCE -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member -I/home/dimitra/eRPC/dpdk/drivers/common/octeontx2/ -I/home/dimitra/eRPC/dpdk/drivers/mempool/octeontx2/ -I/home/dimitra/eRPC/dpdk/drivers/raw/octeontx2_dma/   -D__USE_MISC -D_GNU_SOURCE -Wno-cast-qual -Wno-error -Wno-int-conversion -Wno-unused-parameter -Wno-strict-prototypes -Wno-old-style-definition -Wno-implicit-function-declaration -Wno-nested-externs -Wno-maybe-uninitialized -Wno-unused-function -Wno-array-bounds -Wno-stringop-overflow -Wno-pointer-to-int-cast -I/home/dimitra/eRPC/build/root/include -I/home/dimitra/eRPC/build/root/include/sys -fPIC -o otx2_dpi_rawdev.o -c /home/dimitra/eRPC/dpdk/drivers/raw/octeontx2_dma/otx2_dpi_rawdev.c 
