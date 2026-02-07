#include "ff_port.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

static FILE* emulated_drive = NULL;
static uint64_t emulated_offset = 0;
#define SECTOR_SIZE (0x200)

void ff_init(FILE* fd, uint64_t offset) {
	emulated_drive = fd;
	emulated_offset = offset;
}

DSTATUS disk_initialize(BYTE pdrv) {
	return RES_OK;
}
DSTATUS disk_status(BYTE pdrv) {
	return RES_OK;
}
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
	printf("disk_read: %x %p %x %x\n", pdrv, buff, sector, count);
	if (pdrv == 0) {
		fseek(emulated_drive, emulated_offset + (sector * SECTOR_SIZE), SEEK_SET);
		fread(buff, 1, count * SECTOR_SIZE, emulated_drive);
		return RES_OK;
	}
	else {
		return STA_NOINIT;
	}
}
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
	printf("disk_write: %x %p %x %x\n", pdrv, buff, sector, count);
	if (pdrv == 0) {
		fseek(emulated_drive, emulated_offset + (sector * SECTOR_SIZE), SEEK_SET);
		fwrite(buff, 1, count * SECTOR_SIZE, emulated_drive);
		return RES_OK;
	}
	else {
		return STA_NOINIT;
	}
}
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
	assert(1);
}
uint32_t get_fattime(void) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	return ((tm.tm_year - 1980) << 25) | (tm.tm_mon << 21) | (tm.tm_mday << 16) | (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_min >> 1);
}