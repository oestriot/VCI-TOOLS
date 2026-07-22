#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "aes.h"

void aes_cbc_dec(uint8_t* data, size_t dataLen, const uint8_t* key, uint8_t* iv) {
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_decrypt_buffer(&ctx, data, dataLen);
}

void aes_cbc_enc(uint8_t* data, size_t dataLen, const uint8_t* key, uint8_t* iv) {
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_encrypt_buffer(&ctx, data, dataLen);
}

void aes_ecb_dec(uint8_t* data, const uint8_t* key) {
	struct AES_ctx ctx;
	AES_init_ctx(&ctx, key);
	AES_ECB_decrypt(&ctx, data);
}

void aes_ecb_enc(uint8_t* data, const uint8_t* key) {
	struct AES_ctx ctx;
	AES_init_ctx(&ctx, key);
	AES_ECB_encrypt(&ctx, data);
}

void aes_ecb_out_dec(uint8_t* out, const uint8_t* data, const uint8_t* key) {
	struct AES_ctx ctx;
	memcpy(out, data, AES_BLOCKLEN);

	AES_init_ctx(&ctx, key);
	AES_ECB_decrypt(&ctx, out);
}

void aes_ecb_out_enc(uint8_t* out, const uint8_t* data, const uint8_t* key) {
	struct AES_ctx ctx;
	memcpy(out, data, AES_BLOCKLEN);

	AES_init_ctx(&ctx, key);
	AES_ECB_encrypt(&ctx, out);
}