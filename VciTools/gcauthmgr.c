#include "vci.h"
#include "sha1.h"
#include "sha256.h"

void get_cart_hash(GcCmd56Keys* keys, uint8_t* packet20_hash) {
	SHA1_CTX sha1;
	sha1_init(&sha1);
	sha1_update(&sha1, keys->packet20_key, sizeof(keys->packet20_key));
	sha1_final(&sha1, packet20_hash);
}

void get_cart_secret(GcCmd56Keys* keys, uint8_t* cart_secret) {
	SHA256_CTX sha256;
	sha256_init(&sha256);
	sha256_update(&sha256, keys, sizeof(GcCmd56Keys));
	sha256_final(&sha256, cart_secret);
}