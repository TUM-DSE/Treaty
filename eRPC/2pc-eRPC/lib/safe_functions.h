#pragma once

#include <memory>
#include <cstring>
#include "encrypt_package.h"

extern std::shared_ptr<PacketSsl> txn_cipher;


std::unique_ptr<char[]> safe_memcpy(void* data, size_t size);
std::unique_ptr<uint8_t[]> safe_decryption(char* enc_data, size_t enc_size);
std::unique_ptr<uint8_t[]> safe_encryption(const char* msg, size_t msg_size, size_t& enc_size);
size_t sizeof_encrypted_buffer(size_t msg_size);
bool safe_encryption_in_place(uint8_t*& dst, void* src, size_t src_size);
