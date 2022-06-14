#include "util.h"

#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <memory> // needed for std::unique_ptr


#include <google/protobuf/io/coded_stream.h>          
#include <google/protobuf/io/zero_copy_stream_impl.h>

#ifdef SCONE
#include "server_app_scone/message.pb.h"
#else
#include "server_app/message.pb.h"
#endif

std::string getKeyTobeRead(char* msg) {
	char buf[9];
	::memcpy(buf, &msg[17], 8);
	buf[8] = '\0';
	int key_size = std::atoi(buf);

	std::unique_ptr<char[]> kBuffer(new char[key_size]);
	::memcpy(kBuffer.get(), &msg[25], key_size);
	std::string key(kBuffer.get(), key_size);
	return key;
}

std::string extractKey(char* key_msg) {
	char buf[9];

	::memcpy(buf, &key_msg[17], 8);
	buf[8] = '\0';
	int key_size = std::atoi(buf);

	std::unique_ptr<char[]> kBuffer = std::make_unique<char[]>(key_size +1);
	::memcpy(kBuffer.get(), key_msg + 25, key_size);
	kBuffer[key_size] = '\0';

	std::string key(kBuffer.get(), key_size);


	return key;
}


std::vector<std::string> splitKVPair(const char* keyValuePair) {
	char buf[9];
	const char* msg = keyValuePair;
	std::vector<std::string> vec;

	::memcpy(buf, &msg[17], 8);
	buf[8] = '\0';
	int key_size = std::atoi(buf);

	::memcpy(buf, &msg[25], 8);
	buf[8] = '\0';
	int value_size = std::atoi(buf);  

	std::unique_ptr<char[]> kBuffer = std::make_unique<char[]>(key_size +1);
	std::unique_ptr<char[]> vBuffer = std::make_unique<char[]>(value_size +1);
	::memcpy(kBuffer.get(), msg + 33, key_size);
	::memcpy(vBuffer.get(), msg + 33 + key_size, value_size);
	kBuffer[key_size] = '\0';
	vBuffer[value_size] = '\0';

	std::string key(kBuffer.get(), key_size);
	std::string value(vBuffer.get(), value_size);

	vec.push_back(key);
	vec.push_back(value);

	/* for debugging
	{
		google::protobuf::Arena arena;
		tutorial::ClientMessageResp _resp;
		tutorial::ClientMessageResp* resp = _resp.New(&arena);
		std::string* add_string = resp->add_readvalues();
		add_string->assign(key);
		add_string = resp->add_readvalues();
		add_string->assign(value);

		tutorial::Message proto_msg;
		proto_msg.set_messagetype(tutorial::Message::ClientRespMessage);
		proto_msg.set_allocated_clientrespmsg(resp);

		std::cout << "Inside split KV-pair:: (key " << key_size << ", value_size " << value_size << ")" << proto_msg.DebugString() << "\n";

	}
	*/


	return vec;
}


bool success(const char* msg) {
	std::string tmp(msg);
	return (tmp.find("ACK") != std::string::npos) ? true : false;
}

int getTxnId(const char* msg) {
	char id[9];
	::memcpy(id, msg+8, 8);
	id[8] = '\0';

	return std::atoi(id);
}

int getCoordinatorId(const char* msg) {
	char coordinator[9];
	::memcpy(coordinator, msg, 8);
	coordinator[8] = '\0';

	return std::atoi(coordinator);
}
