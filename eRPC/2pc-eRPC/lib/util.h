#include <vector>
#include <stdlib.h>
#include <cstring>
#include <string>

// TODO: comments
std::string getKeyTobeRead(char*);

std::vector<std::string> splitKVPair(const char*);

std::string extractKey(char* key_msg);

bool success(const char*);

int getCoordinatorId(const char*);

int getTxnId(const char*);
