#include <cassert>
#include <iostream>
#include <errno.h>
#include <malloc.h>
#include <numaif.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdexcept>
#include <vector>
#include <cstring>

#define SHM_KEY 3185
#define BUF_SIZE (64 * 1024 * 1024)

int main(int argc, char **argv) {
	int i;
	int shmid = shmget(SHM_KEY, BUF_SIZE, IPC_CREAT | 0666 | SHM_HUGETLB);
	uint8_t *buf = static_cast<uint8_t *>(shmat(shmid, 0, 0));

	assert(argc == 2);
	int preferred_node = atoi(argv[1]);
	const unsigned long nodemask = (1 << preferred_node);
	printf("\tNodemask = %ld\n", nodemask);

	int _ret = mbind(buf, BUF_SIZE, 
		MPOL_BIND,		/** < A strict assignment to nodes in nodemask */
		&nodemask,		/** < A bitmask of valid NUMA nodes */
		32,				/** < Num. of bits to consider. XXX: 2 does not work */
		0);				/** < Mode ?? */
	assert(_ret >= 0);
	
	for(i = 0; i < BUF_SIZE; i ++) {
		buf[i] = (char) rand();
	}

	printf("\tRandom pick = %c\n", buf[rand() % BUF_SIZE]);
}
