#include "vci.h"
#include "sha1.h"
#include "sha256.h"

void get_cart_hash(const GcCmd56Keys* keys, uint8_t* packet20_hash) {
	SHA1_CTX sha1;
	sha1_init(&sha1);
	sha1_update(&sha1, (uint8_t*)keys->packet20_key, sizeof(keys->packet20_key));
	sha1_final(&sha1, packet20_hash);
}

void get_cart_secret(const GcCmd56Keys* keys, uint8_t* cart_secret) {
	Sha256Context sha256;
	Sha256Initialise(&sha256);
	Sha256Update(&sha256, keys, sizeof(GcCmd56Keys));
	Sha256Finalise(&sha256, (SHA256_HASH*)cart_secret);
}