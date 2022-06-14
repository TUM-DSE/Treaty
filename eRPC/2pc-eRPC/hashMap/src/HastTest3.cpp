#include <string>
#include <thread>
#include "HashMap.h"

std::unique_ptr<char[]> format_index_identifier(int currNodeId, int recipientId, int txnId, int indexOp) {
	size_t alloc_size = 32 + 1;
	std::unique_ptr<char[]> indexing = std::make_unique<char[]>(alloc_size);

	size_t offset = 0;
	char *hello = reinterpret_cast<char*>(indexing.get());
	offset += snprintf(hello+offset, alloc_size-offset, "%08d", currNodeId);
	offset += snprintf(hello+offset, alloc_size-offset, "%08d", recipientId);
	offset += snprintf(hello+offset, alloc_size-offset, "%08d", txnId);
	offset += snprintf(hello+offset, alloc_size-offset, "%08d", indexOp);
	hello[alloc_size - 1] = '\0';
	// std::cout << "indexing: " << indexing.get() << " " <<  currNodeId << " " << recipientId << " " << txnId << " " << indexOp << "\n";
	// return std::move(indexing);
	return indexing;

};



int main() {

	//Multi threaded test with two threads
	CTSL::HashMap<std::string, std::string> Map;
	auto _findex = format_index_identifier(0, 0, 1, 1);
	std::string st(_findex.get());
	for (int i = 0 ; i < 100; i ++) {
		_findex = format_index_identifier(0, 0, 1, i);
		st = _findex.get();
		Map.insert(st, st);
	}


	return 0;

}

