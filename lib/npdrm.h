#ifndef _NPDRM_H
#define _NPDRM_H 
#include "compat.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

typedef PACK(struct SceNpBindData {
	uint8_t cart_secret[0x20];
	uint8_t license[0x70];
}) SceNpBindData;

typedef PACK(struct SceNpDrmLicense { // size is 0x200
	int16_t version;       // -1 VITA (NPDRM_FREE), 0 PSP, 1 PSP-VITA
	int16_t version_flags; // 0, 1
	int16_t license_type;  // 1
	int16_t license_flags; // 0x400:non-check ecdsa
	uint64_t account_id;   // 0x0:NPDRM_FREE
	char content_id[0x30];
	uint8_t key_table[0x10];
	uint8_t key1[0x10];
	uint64_t start_time;
	uint64_t expiration_time;
	uint8_t ecdsa_signature[0x28];
	int64_t flags;
	uint8_t key2[0x10];
	uint8_t unk_0xB0[0x10];
	uint8_t open_psid[0x10];
	uint8_t unk_0xD0[0x10];
	uint8_t cmd56_handshake_part[0x14];
	int debug_upgradable;
	int unk_0xF8;
	int sku_flag;
	uint8_t rsa_signature[0x100];
}) SceNpDrmLicense;
static_assert(sizeof(SceNpDrmLicense) == 0x200, "Invalid size for SceNpDrmLicense");

void decrypt_klicensee(uint8_t* rif_key, uint8_t* klicensee, SceNpDrmLicense* license);

#endif