#include <atomic>
#include <sys/time.h>

// std::atomic<uint64_t> commits_requested = 0;
// std::atomic<uint64_t> commits_served = 0;

unsigned long get_time() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        unsigned long ret = tv.tv_usec;
        ret /= 1000;
        ret += (tv.tv_sec * 1000);
        return ret;
}
