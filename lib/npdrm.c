#include "npdrm.h"
#include "psv.h"
#include "hmac_sha256.h"
#include "aes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t BIND_KEY[0x10] = { 0x90, 0x1a, 0x84, 0xfb, 0x13, 0xa7, 0x44, 0xa3, 0x78, 0xc5, 0x01, 0x8a, 0x60, 0xf5, 0x8c, 0x22 };


void decrypt_klicensee(uint8_t* cart_secret, uint8_t* klicensee, SceNpDrmLicense* license) {
	struct AES_ctx ctx;
	SceNpBindData bind_data = { 0 };
	uint8_t bind_hmac[0x20] = { 0 };
	uint8_t aes_key[0x10] = { 0 };
	uint8_t aes_iv[0x10] = { 0 };

	memcpy(bind_data.cart_secret, cart_secret, sizeof(bind_data.cart_secret));
	memcpy(bind_data.license, license, sizeof(bind_data.license));
	
	hmac_sha256(BIND_KEY, sizeof(BIND_KEY), &bind_data, sizeof(bind_data), bind_hmac, sizeof(bind_hmac));
	
	memcpy(aes_key, bind_hmac + 0x00, sizeof(aes_key));
	memcpy(aes_iv, bind_hmac + 0x10, sizeof(aes_iv));

	AES_init_ctx_iv(&ctx, aes_key, aes_iv);
	memcpy(klicensee, license->key2, CART_SECRET_SIZE);
	AES_CBC_decrypt_buffer(&ctx, klicensee, CART_SECRET_SIZE);

}