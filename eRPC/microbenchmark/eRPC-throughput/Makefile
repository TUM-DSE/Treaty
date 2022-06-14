
.PHONY = all simple_server simple_client

export eRPC_DIR = /home/dimitra/eRPC
export DPDK = ${eRPC_DIR}/dpdk/x86_64-native-linuxapp-gcc
export HOME_DIR = /home/dimitra/eRPC/microbenchmark/eRPC-throughput

LIBS = -L$(eRPC_DIR)/build -lerpc_nat -pthread -ldl -lnuma  -L$(DPDK)/lib -Wl,--whole-archive -ldpdk  -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port

INCLUDES = -I$(DPDK)/include -Wno-address-of-packed-member -Wno-pedantic -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -std=c++17 -g -I$(eRPC_DIR)/build/root/include -I$(HOME_DIR) -I$(DPDK)/include/dpdk -I$(eRPC_DIR)/lib -I$(eRPC_DIR)/src

SRC_APPS_FILES_PATH=/home/dimitra/eRPC/microbenchmark/eRPC-throughput

SRC_FILES = $(HOME_DIR)/stats/stats.cc

all: dpdk

dpdk: simple_client simple_server

simple_server: 
	g++ -DERPC_DPDK=true -Wall -Wextra -Wpedantic -Wno-deprecated-declarations -Wno-register -Wno-vla ${INCLUDES} -march=native -include $(eRPC_DIR)/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -O3 -W -Wall ${SRC_FILES} ${SRC_APPS_FILES_PATH}/udp_sender.cc $(LIBS)  -o sender-clara


simple_client: 
	g++ -DERPC_DPDK=true -Wall -Wextra -Wpedantic -Wno-deprecated-declarations -Wno-register -Wno-vla ${INCLUDES} -march=native -include $(eRPC_DIR)/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -O3 -W -Wall ${SRC_FILES} ${SRC_APPS_FILES_PATH}/udp_listener.cc $(LIBS)  -o listener-amy

clean:
	-rm -f listener-amy sender-clara
