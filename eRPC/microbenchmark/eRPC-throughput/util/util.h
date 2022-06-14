#include <stdio.h>
#include <inttypes.h>
#include <cassert>

static const char alphanum[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";

static int stringLength = sizeof(alphanum) - 1;

static char genRandom() {
        return alphanum[rand() % stringLength];
}

std::string randomKey(size_t size) {
        std::string Str;
        for (unsigned int i = 0; i < size; ++i) {
                Str += genRandom();
        }
        return Str;
}

std::string messageWithTimestamp(uint64_t sent_msgs, size_t kMsgSize) {
	std::unique_ptr<char[]> buff(new char[sizeof(uint64_t) + 1]);
	sprintf(buff.get(), "%" PRIu64, sent_msgs);
	buff.get()[sizeof(uint64_t)] = '\0';
	return std::string(buff.get());
}

template <typename T>
T find_max_time(T* times, size_t len) {
	assert(len > 0);
	T max_time = times[0];
	for (size_t i = 0; i < len; i++) {
		if (times[i] > max_time)
			max_time = times[i];
	}

	return max_time;
}
