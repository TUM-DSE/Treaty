#include "safe_functions.h"


std::unique_ptr<char[]> safe_memcpy(void* data, size_t size) {
	std::unique_ptr<char[]> ptr = std::make_unique<char[]>(size+1);
	::memcpy(ptr.get(), data, size);
	ptr.get()[size] = '\0';
	return std::move(ptr);
}


std::unique_ptr<uint8_t[]> safe_decryption(char* enc_data, size_t enc_size) {
	size_t dec_size = PacketSsl::get_message_size(enc_size);
	std::unique_ptr<uint8_t[]> ptr = std::make_unique<uint8_t[]>(dec_size);

	//bool ok = txn_cipher->decrypt(ptr.get(), dec_size, enc_data, enc_size);
	bool [[maybe_unused]] ok = txn_cipher->decrypt(ptr.get(), enc_data, enc_size);
	assert(ok);

	return std::move(ptr);
}


size_t sizeof_encrypted_buffer(size_t msg_size) {
	return PacketSsl::get_buffer_size(msg_size);
}


bool safe_encryption_in_place(uint8_t*& dst, void* src, size_t src_size) {
        size_t enc_size = PacketSsl::get_buffer_size(src_size);
	//bool ok = txn_cipher->encrypt(dst, enc_size, src, src_size);
	bool ok = txn_cipher->encrypt(dst, src, src_size);
	return ok;
}


std::unique_ptr<uint8_t[]> safe_encryption(const char* msg, size_t msg_size, size_t& enc_size) {
        enc_size = PacketSsl::get_buffer_size(msg_size);
        std::unique_ptr<uint8_t[]> enc_buf = std::make_unique<uint8_t[]>(enc_size);
        //bool ok = txn_cipher->encrypt(enc_buf.get(), enc_size, msg, msg_size);
        bool [[maybe_unused]] ok = txn_cipher->encrypt(enc_buf.get(), msg, msg_size);
	assert(ok);

	return std::move(enc_buf);
}

