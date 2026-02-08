#ifndef VCI_H 
#define VCI_H 1

#include <stdint.h>
#include <assert.h>
#include "compat.h"


#define VCI_MAGIC "VCI\0"
#define VCI_MAJOR_VER 1
#define VCI_MINOR_VER 0

typedef PACK(struct GcCmd56Keys {
	uint8_t packet20_key[0x20];
	uint8_t packet18_key[0x20];
}) GcCmd56Keys;
static_assert(sizeof(GcCmd56Keys) == 0x40, "Invalid Size for GcCmd56Keys");

typedef PACK(struct VciHeader {
	
	char magic[0x4]; // 0x4
	uint16_t major_version; // 0x6
	uint16_t minor_version; // 0x8
	uint64_t device_size; // 0x10
	GcCmd56Keys keys; // 0x50
	
	uint8_t padding[0x1B0]; // 0x200
}) VciHeader;
static_assert(sizeof(VciHeader) == 0x200, "Invalid Size for VciHeader");

#endif