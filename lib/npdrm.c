#include "npdrm.h"
#include "psv.h"
#include "hmac_sha256.h"
#include "crypto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t BIND_KEY[0x10] = { 0x90, 0x1a, 0x84, 0xfb, 0x13, 0xa7, 0x44, 0xa3, 0x78, 0xc5, 0x01, 0x8a, 0x60, 0xf5, 0x8c, 0x22 };
const uint8_t PSP_RIF_KEY[0x10] = { 0xDA, 0x7D, 0x4B, 0x5E, 0x49, 0x9A, 0x4F, 0x53, 0xB1, 0xC1, 0xA1, 0x4A, 0x74, 0x84, 0x44, 0x3B };
const uint8_t ACT_KEY[0x10] = { 0x5E, 0x06, 0xE0, 0x4F, 0xD9, 0x4A, 0x71, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
const uint8_t VITA_RIF_KEY[0x20] = { 0x33, 0xE9, 0x51, 0xF7, 0xEC, 0xDA, 0x7D, 0xDA, 0xEA, 0x48, 0x35, 0xFE, 0x2F, 0x36, 0xD0, 0x5B, 0xC8, 0xEC, 0xB7, 0x3F, 0x99, 0x70, 0xD2, 0x91, 0x78, 0xF3, 0x96, 0xF9, 0x9C, 0x89, 0xFC, 0x1F };

// npdrm variables;
static SceNpDrmActivationData NPDRM_ACTIVATION_DATA = { 0 };
static uint8_t CART_SECRET[0x20] = { 0 };
static uint8_t CONSOLE_ID[0x10] = { 0 };
static uint8_t ACTIVATED = 0;


int decrypt_keyid(uint8_t* key_id_block) {
	aes_ecb_dec(key_id_block, PSP_RIF_KEY);

	int key_id = 0;
	memcpy(&key_id, key_id_block + 0xC, sizeof(key_id));

	return key_id;
}

void decrypt_key_table() {

	// generate per console act key.
	uint8_t act_key_perconsole[0x10] = { 0 };
	aes_ecb_out_enc(act_key_perconsole, ACT_KEY, CONSOLE_ID);

	// decrypt all activation keys
	for (int i = 0; i < sizeof(NPDRM_ACTIVATION_DATA.primary_key_table) / 0x10; i++) {
		aes_ecb_dec(NPDRM_ACTIVATION_DATA.primary_key_table[i], act_key_perconsole);
	}

	ACTIVATED = 1;
}

void init_npdrm(uint8_t* cart_secret, uint8_t* console_id, SceNpDrmActivationData* activation_data) {
	if (cart_secret != NULL) { memcpy(CART_SECRET, cart_secret, sizeof(CART_SECRET)); }
	if (console_id != NULL) memcpy(CONSOLE_ID, console_id, sizeof(CONSOLE_ID));
	if (activation_data != NULL) memcpy(&NPDRM_ACTIVATION_DATA, activation_data, sizeof(NPDRM_ACTIVATION_DATA));

	if (activation_data != NULL && console_id != NULL) decrypt_key_table();
}


void set_cart_secret(uint8_t* cart_secret) {
	memcpy(CART_SECRET, cart_secret, sizeof(CART_SECRET));
}

void set_console_id(uint8_t* console_id) {
	memcpy(CONSOLE_ID, console_id, sizeof(CONSOLE_ID));
}

void set_activation_data(uint8_t* activation_data) {
	memcpy(&NPDRM_ACTIVATION_DATA, activation_data, sizeof(NPDRM_ACTIVATION_DATA));
}

void decrypt_bind_data(char* key, size_t keyLen, char* bindData, size_t bindDataLen) {
	uint8_t bind_hmac[0x20] = { 0 };
	uint8_t aes_key[0x10] = { 0 };
	uint8_t aes_iv[0x10] = { 0 };

	hmac_sha256(BIND_KEY, sizeof(BIND_KEY), bindData, bindDataLen, bind_hmac, sizeof(bind_hmac));

	memcpy(aes_key, bind_hmac + 0x00, sizeof(aes_key));
	memcpy(aes_iv, bind_hmac + 0x10, sizeof(aes_iv));

	aes_cbc_dec(key, keyLen, aes_key, aes_iv);
}


int decrypt_klicensee(uint8_t* klicensee, SceNpDrmLicense* license, int vita) {
	SceNpBindData bind_data = { 0 };
	
	uint8_t digital_flag = (((license->license_flags & 0xff) << 8) & 0x7ff) >> 10 == 0;
	uint8_t psp_flag = ((vita == 0) & ~((((license->license_flags & 0xff) << 8) & 0x7ff) >> 10)) != 0;
	uint8_t drmbind_flag = (license->license_flags >> 8) != 0b00001101;
	uint8_t debug_flag = (license->license_flags >> 8) == 0;
	
	// copy key2 to klicensee buffer ...
	memcpy(klicensee, license->key2, sizeof(license->key2));

	// copy license to license buffer ..
	memcpy(bind_data.license, license, sizeof(bind_data.license));

	// if not vita, do it the psp way;
	if (!vita && psp_flag) {
		if (!ACTIVATED) return 0;

		int key_id = decrypt_keyid(license->key_table);
		aes_ecb_out_dec(klicensee, license->key1, NPDRM_ACTIVATION_DATA.primary_key_table[key_id]);

		if (debug_flag) {
			memset(klicensee, 0xAA, KLICENSEE_SIZE);
		}

		return 1;
	}

	if (digital_flag) {
		// read activation if npdrm bind;
		if (drmbind_flag) {
			if (!ACTIVATED) return 0;
			memcpy(bind_data.key, NPDRM_ACTIVATION_DATA.primary_key_table, sizeof(bind_data.key));
		}
		else { // if npdrm free, use the rif key directly;
			memcpy(bind_data.key, VITA_RIF_KEY, sizeof(bind_data.key));
		}
	}
	else {
		if (vita == 0) {
			memcpy(klicensee, license->key1, KLICENSEE_SIZE);
		}
		else {
			memcpy(bind_data.key, CART_SECRET, sizeof(bind_data.key));
		}
	}

	if (debug_flag) {
		memset(klicensee, 0x00, KLICENSEE_SIZE);
		return 1;
	}
	
	decrypt_bind_data(klicensee, KLICENSEE_SIZE, (char*)&bind_data, sizeof(SceNpBindData));
	return 1;
}

// NoNpDrm;


int make_nonpdrm_license(SceNpDrmLicense* license, uint8_t* klicensee) {

	SceNpDrmLicense new_license = { 0 };

	new_license.account_id = 0x0123456789ABCDEFLL;
	new_license.version = swap16(1);
	new_license.version_flags = swap16(1);
	new_license.license_type = swap16(1);

	new_license.license_flags = swap16(swap16(license->license_flags) & ~0x400);
	new_license.flags = license->flags;

	if (swap32(license->sku_flag) == 1 || swap32(license->sku_flag) == 3) {
		new_license.sku_flag = swap32(3);
	}
	else {
		new_license.sku_flag = swap32(0);
	}

	memcpy(new_license.content_id, license->content_id, sizeof(license->content_id));
	memcpy(new_license.key1, klicensee, sizeof(new_license.key1));

	memcpy(license, &new_license, sizeof(SceNpDrmLicense));

	return 1;
}