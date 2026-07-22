#ifndef CRYPTO_H
#define CRYPTO_H 1
#include <stdint.h>

void aes_cbc_dec(uint8_t* data, size_t dataLen, const uint8_t* key, uint8_t* iv);
void aes_cbc_enc(uint8_t* data, size_t dataLen, const uint8_t* key, uint8_t* iv);

void aes_ecb_dec(uint8_t* data, const uint8_t* key);
void aes_ecb_enc(uint8_t* data, const uint8_t* key);

void aes_ecb_out_dec(uint8_t* out, const uint8_t* data, const uint8_t* key);
void aes_ecb_out_enc(uint8_t* out, const uint8_t* data, const uint8_t* key);

#endif