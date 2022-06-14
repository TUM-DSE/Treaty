#pragma once

#include <unistd.h>

#define SYS_untrusted_mmap 1025

static void * scone_kernel_mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset) {
	return (void*)syscall(SYS_untrusted_mmap, addr, length, prot, flags, fd, offset);
}
