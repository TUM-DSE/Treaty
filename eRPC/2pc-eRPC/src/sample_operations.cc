#include "sample_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "config.h"
#include <memory>

#define MAXLINE 1024

namespace msg {
	using TxnId = int;
	using CurNodeId = int;

	std::unique_ptr<char []> TxnPutMsg(std::string key, std::string value, TxnId txn_id, CurNodeId nodeId) {
		/*
		char hello[MAXLINE];
		*/
		size_t msg_size = sizeof(char) + 32 + key.size() + value.size() + 1;
		std::unique_ptr<char[]> temp_buf = std::make_unique<char []>(msg_size);
		::memset(temp_buf.get(), '\0', msg_size);
		temp_buf[msg_size - 1] = '\0';
		size_t offset = 0;

		temp_buf[0] = 'D';
		offset += sizeof(char);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset, "%08d", nodeId);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset, "%08d", txn_id);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset,"%08d", static_cast<int>(key.size()));
		offset += snprintf(temp_buf.get()+offset, msg_size-offset,"%08d", static_cast<int>(value.size()));
		::memcpy(temp_buf.get() + offset, key.c_str(), key.size());
		offset += key.size();
		::memcpy(temp_buf.get() + offset, value.c_str(), value.size());
		/*
		offset += snprintf(temp_buf.get()+offset, msg_size-offset,"%s", key.c_str());
		offset += snprintf(temp_buf.get()+offset, msg_size-offset,"%s", value.c_str());
		*/
		std::string st(temp_buf.get());



		/*
		hello[0] = 'D';
		offset += sizeof(char);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", nodeId);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_id);
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%08d", static_cast<int>(key.size()));
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%08d", static_cast<int>(value.size()));
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%s", key.c_str());
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%s", value.c_str());
		hello[offset] = '\0';
		*/
		return std::move(temp_buf);
	}

	std::unique_ptr<char []> TxnDeleteMsg(std::string key, TxnId txn_id, CurNodeId nodeId) {
		size_t msg_size = sizeof(char) + 24 + key.size() + 1;
		std::unique_ptr<char[]> temp_buf = std::make_unique<char []>(msg_size);
		::memset(temp_buf.get(), '\0', msg_size);
		temp_buf[msg_size - 1] = '\0';
		size_t offset = 0;

		temp_buf[0] = 'K';
		offset += sizeof(char);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset, "%08d", nodeId);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset, "%08d", txn_id);
		offset += snprintf(temp_buf.get()+offset, msg_size-offset,"%08d", static_cast<int>(key.size()));
		::memcpy(temp_buf.get() + offset, key.c_str(), key.size());



		return std::move(temp_buf);
	}


	std::string TxnReadMsg(std::string& key, TxnId txn_id, CurNodeId nodeId) {
		char hello[MAXLINE];
		size_t offset = 0;

		hello[0] = 'R';
		offset += sizeof(char);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", nodeId);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_id); 
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%08d", static_cast<int>(key.size()));
		offset += snprintf(hello+offset, sizeof(hello)-offset,"%s", key.c_str());
		hello[offset] = '\0';

		return std::string(hello);
	}

	std::string TxnPrepMsg(TxnId txn_id, CurNodeId nodeId) {
		char hello[MAXLINE];
		size_t offset = 0;

		//Message Format <coordinator_id> <coordinator's TxnID>
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", nodeId);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_id); 
		hello[offset] = '\0';
		return std::string(hello);
	}

	std::string TxnCommitMsg(TxnId txn_id, CurNodeId nodeId) {
		char hello[MAXLINE];
		size_t offset = 0;

		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", nodeId);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_id); 
		hello[offset] = '\0';

		return std::string(hello);
	}

	std::string TxnBeginMsg(TxnId txn_id, CurNodeId nodeId) {
		char hello[MAXLINE];
		size_t offset = 0;

		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", nodeId);
		offset += snprintf(hello+offset, sizeof(hello)-offset, "%08d", txn_id); 
		hello[offset] = '\0';

		return std::string(hello);
	}

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

	std::string getVal(size_t size) {
		std::string Str;
		for (unsigned int i = 0; i < size; ++i) {
			Str += alphanum[1];
		}
		return Str;
	}

} // namespace msg
