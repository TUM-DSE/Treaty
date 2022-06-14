#pragma once

#include "encrypt_openssl.h"

#include "utility.h"
#include <cassert>
#include <memory>
#include <utility>
#include <iostream>

struct KeyIV {
	public:
		static size_t constexpr KeySize = 16;
		static size_t constexpr IvSize = 12;
		alignas(16) std::byte  key[16];
		alignas(16) mutable std::byte iv[16];

		KeyIV(std::byte const * key, std::byte const * iv) {
			assert(reinterpret_cast<uintptr_t>(this->iv) % 16 == 0);
			memcpy(this->key, key, KeySize);
			memcpy(this->iv, iv, IvSize);
#if DEBUG
			memset(this->iv + IvSize, 0, 16 - IvSize);
#endif
		}

	private:
		template<class INT>
			static void inc(std::byte * dst, std::byte * src, INT const a) noexcept {
				struct __attribute__((packed)) Val {
					uint64_t low;
					uint32_t high;
				};
				auto dst_val = reinterpret_cast<Val *>(dst);
				auto src_val = reinterpret_cast<Val *>(src);
				asm (
						"add   %[a], %[low]\n"
						"adc   $0,   %[high]\n"
						: [low]  "=rm"    (dst_val->low),
						[high] "=rm"    (dst_val->high)
						: [a]    "erm"    (a),
						"[low]"  (src_val->low),
						"[high]" (src_val->high)
						: "cc"
				    );
			}

	public:

		void get_packet_iv(std::byte * dst) const noexcept {
			assert((reinterpret_cast<uintptr_t>(dst) & 0xF) == 0);
			alignas(16) std::byte raw[16];
			{
				inc(raw, dst, 1);
			} while (!shieldstore_::compare_exchange_128(iv, raw, dst));
		}
};

/**
 * Can de-/encrypt packet buffers the data structure of an encrypted buffer will look like
 * +-------+------+-----------+-------+
 * |   IV  | size |    data   |  MAC  |
 * +-------+------+-----------+-------+
 * 0       12    16          n-16     n
 *
 */
class PacketSsl {
	private:
		std::shared_ptr<KeyIV> key_iv;

	public:
		enum Func {
			DECRYPT = 0,
			ENCRYPT = 1
		};

		using Size_t = uint32_t;

		struct Ret_val {
			bool valid;
			Size_t size;

			constexpr operator bool() const noexcept { return valid; }
			constexpr operator std::pair<bool,uint32_t>() { return {valid, size}; }
		};

		static size_t constexpr KeySize = CipherSsl::KeySize;
		static size_t constexpr IvSize  = CipherSsl::IvSize;
		static size_t constexpr MacSize = CipherSsl::MacSize;
		static size_t constexpr BlockBits = CipherSsl::BlockBits;
		static size_t constexpr BlockSize = CipherSsl::BlockSize;
		static size_t constexpr BlockMask = CipherSsl::BlockMask;

		explicit PacketSsl(std::shared_ptr<KeyIV> key_iv) : key_iv(std::move(key_iv)) {}

		static PacketSsl create(std::byte const * key, std::byte const * iv) {
			return PacketSsl(std::make_shared<KeyIV>(key, iv));
		}

		static PacketSsl create(CipherSsl const & cipher) {
			return PacketSsl(std::make_shared<KeyIV>(reinterpret_cast<std::byte const *>(cipher._key), reinterpret_cast<std::byte const *>(cipher._iv)));
		}

		//returns the minimum size the buffer for encryption must have
		[[gnu::pure]]
			static size_t constexpr get_buffer_size(size_t const msg_sz) noexcept {
				auto sz = msg_sz + sizeof(Size_t);
				size_t res = (sz & BlockMask) == 0 ? sz : sz + (BlockSize - (sz & BlockMask));
				res += IvSize + MacSize + sizeof(Size_t);
				return res;
			}

		//returns the maximum size the message can have with the given data_sz
		[[gnu::pure]]
			static size_t constexpr get_message_size(size_t const buf_sz) noexcept {
				return buf_sz - IvSize - MacSize - sizeof(Size_t);
			}

		[[gnu::pure]]
			static size_t constexpr adjust_size(size_t const msg_sz) noexcept(noexcept(get_buffer_size(msg_sz))) { return get_buffer_size(msg_sz); }

	private:

		template<Func func>
			static bool cipher_final(EVP_CIPHER_CTX * pState, unsigned char * p_dst, int * len, unsigned char * mac) noexcept {
				if constexpr (func == ENCRYPT) {
					return EVP_CipherFinal_ex(pState, p_dst, len)
						&& EVP_CIPHER_CTX_ctrl(pState, EVP_CTRL_GCM_GET_TAG, MacSize, mac);
				}
				return EVP_CIPHER_CTX_ctrl(pState, EVP_CTRL_GCM_SET_TAG, MacSize, mac)
					&& EVP_CipherFinal_ex(pState, p_dst, len);
			}

		template<Func func>
			static bool cipher_update(EVP_CIPHER_CTX * pState, unsigned char * p_dst, int * len, unsigned char * p_src, Size_t & size) {
				if constexpr (func == ENCRYPT) {
					return EVP_CipherUpdate(pState, p_dst, len, reinterpret_cast<unsigned char const *>(&size), sizeof(size))
						&& EVP_CipherUpdate(pState, p_dst + *len, len, p_src, size);
				}
				auto max_size = size;
				size = 0;
				return EVP_CipherUpdate(pState, reinterpret_cast<unsigned char *>(&size), len, p_src, sizeof(size))
					&& (size <= max_size) 
					&& EVP_CipherUpdate(pState, p_dst, len, p_src + *len, size);
			}

	public:

		//En-/decrypts a buffer, the function is thread safe
		template<Func func, class DST, class SRC>
			Ret_val cipher(DST * dst, SRC * src, Size_t size) const noexcept {
				int len = 0;
				auto p_dst = reinterpret_cast<unsigned char *>(const_cast<typename std::remove_const<DST>::type *>(dst));
				auto p_src = reinterpret_cast<unsigned char *>(const_cast<typename std::remove_const<SRC>::type *>(src));

#if 0 //with C++20 we could do it like this
				std::unique_ptr<EVP_CIPHER_CTX, decltype([](EVP_CIPHER_CTX * p) {
						if (p != nullptr) {
						EVP_CIPHER_CTX_free(p);
						}
						})> pState;
#endif

#if 0
				EVP_CIPHER_CTX * pState = nullptr;
				auto _f = speicher::finally([&pState] {
						if (pState != nullptr) {
						EVP_CIPHER_CTX_free(pState);
						}
						});
#endif
				ERR_clear_error();

				unsigned char * iv;
				unsigned char * mac;
				if constexpr (func == ENCRYPT) {
					iv = p_dst;
					mac = p_dst + get_buffer_size(size) - MacSize;
					p_dst += IvSize;
					key_iv->get_packet_iv(reinterpret_cast<std::byte*>(iv));
				} else {
					iv = p_src;
					mac = p_src + size - MacSize;
					p_src += IvSize;
				}

				struct CTX_DELETER {
					void operator()(EVP_CIPHER_CTX * p) const {
						if (p != nullptr) {
							EVP_CIPHER_CTX_free(p);
						}
					}
				};

				std::unique_ptr<EVP_CIPHER_CTX, CTX_DELETER> pState(EVP_CIPHER_CTX_new());

				return  {pState
					&& EVP_CipherInit_ex(pState.get(), EVP_aes_128_gcm(), nullptr, reinterpret_cast<unsigned char const *>(key_iv->key), iv, func)
						&& cipher_update<func>(pState.get(), p_dst, &len, p_src, size)
						&& cipher_final<func>(pState.get(), p_dst, &len, mac)
						, size };
			}

		//en-/decrypts message/buffer
		//equivalent to calling encrypt or decrypt however with the func parameter as determinator for encryption or decryption
		template<class DST, class SRC>
			auto cipher(DST * dst, SRC * src, Size_t src_size, Func func) const noexcept -> decltype(cipher<ENCRYPT>(dst, src, src_size)) {
				if (func == ENCRYPT) {
					return cipher<ENCRYPT>(dst, src, src_size);
				}
				return cipher<DECRYPT>(dst, src, src_size);
			}

		//encrypts a message, thread safe
		template<class DST, class SRC>
			auto encrypt(DST * dst, SRC * src, Size_t src_size) const noexcept -> decltype(cipher<ENCRYPT>(dst, src, src_size)) {
				return cipher<ENCRYPT>(dst, src, src_size);
			}

		template<class DST, class SRC>
			auto encrypt(DST * dst, Size_t dest_size, SRC * src, Size_t src_size) const noexcept -> decltype(cipher<ENCRYPT>(dst, src, src_size)) {
				return cipher<ENCRYPT>(dst, src, src_size);
			}

		template<class DST, class SRC>
			auto decrypt(DST * dst, Size_t dest_size, SRC * src, Size_t src_size) const noexcept -> decltype(cipher<ENCRYPT>(dst, src, src_size)) {
				return cipher<DECRYPT>(dst, src, src_size);
			}
		//decyrpts a buffer, thread safe
		template<class DST, class SRC>
			auto decrypt(DST * dst, SRC * src, Size_t src_size) const noexcept -> decltype(cipher<DECRYPT>(dst, src, src_size)) {
				return cipher<DECRYPT>(dst, src, src_size);
			}

};

