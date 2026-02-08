#ifndef PSV_H 
#define PSV_H
#include "compat.h"

#define PSV_MAGIC "PSV\0"
#define PSV_VER (0x1)
#define PSV_FLAGS (0x0)

#define CART_SECRET_SIZE (0x20)
#define CART_HASH_SIZE (0x14)

typedef PACK(struct PsvHeader {
	char magic[4];
	uint32_t version;
	uint32_t flags;
	uint8_t cart_secret[CART_SECRET_SIZE];
	uint8_t cart_hash[CART_HASH_SIZE];
	uint8_t all_sectors_sha256[0x20];
	uint64_t image_size;
	uint64_t image_offset;
	uint8_t padding[0x190];
}) PsvHeader;
static_assert(sizeof(PsvHeader) == 0x200, "Invalid size for PsvHeader.");

#endif // PSV_H