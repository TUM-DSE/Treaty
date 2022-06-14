#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cassert>
#include <errno.h>
#include <iostream>
#include <numaif.h>



int main(int args, char* argv[]) {
	const size_t kHugepageSize = 1024*1024*1024;
	size_t size = kHugepageSize;
	int shm_key, shm_id;

	// Choose a positive SHM key. Negative is fine but it looks scary in the
	// error message.
	shm_key = 3123;
	shm_key = std::abs(shm_key);

	// Try to get an SHM region
	shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | 0666 | SHM_HUGETLB);

	if (shm_id == -1) {    
		switch (errno) {
			case EEXIST:
				break;  // shm_key already exists. Try again.

			case EACCES:
				break;
			case EINVAL:
				break;
			case ENOMEM:
				// Out of memory - this is OK
				std::cout << "eRPC HugeAlloc: Insufficient hugepages. Can't reserve MB.\n";
		}
	}


	uint8_t *shm_buf = static_cast<uint8_t *>(shmat(shm_id, nullptr, 0));
	assert((shm_buf != nullptr) && "eRPC HugeAlloc: shmat() failed. Key = ");

	// Mark the SHM region for deletion when this process exits
	shmctl(shm_id, IPC_RMID, nullptr);

	// Bind the buffer to the NUMA node
	int numa_node = 0;
	const unsigned long nodemask = (1ul << static_cast<unsigned long>(numa_node));
	long ret = mbind(shm_buf, size, MPOL_BIND, &nodemask, 32, 0);
	if (ret == 0)
		std::cout << "eRPC HugeAlloc: mbind() failed. Key " + std::to_string(shm_key) + " size : " + std::to_string(size) + " ";

	return 0;
}
