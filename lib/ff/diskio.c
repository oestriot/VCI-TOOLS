#include "diskio.h"
#include "ff.h"
#include "ffconf.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

static FILE* emulated_drive = NULL;
static uint64_t emulated_offset = 0;
static uint64_t emulated_size = 0;

#define SECTOR_SIZE (0x200)

void ff_init(FILE* fd, uint64_t offset, uint64_t size) {
	emulated_drive = fd;
	emulated_offset = offset;
	emulated_size = size;
}

void seek_64bit(FILE* fd, uint64_t offset) {
#ifdef _WIN32
	_fseeki64(emulated_drive, offset, SEEK_SET);
#elif __linux__
	fseeko(emulated_drive, offset, SEEK_SET);
#else // fallback on long seek loop
	if (offset > LONG_MAX) {
		int total_seeks = offset / LONG_MAX;
		int remain_seek = offset % LONG_MAX;

		fseek(emulated_drive, 0, SEEK_SET);
		for (int i = 0; i < total_seeks; i++) {
			fseek(emulated_drive, LONG_MAX, SEEK_CUR);
		}
		fseek(emulated_drive, remain_seek, SEEK_CUR);
	}
	else {
		fseek(emulated_drive, offset, SEEK_SET);
	}
#endif
}

DSTATUS disk_initialize(BYTE pdrv) {
	return RES_OK;
}
DSTATUS disk_status(BYTE pdrv) {
	return RES_OK;
}
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
	if (pdrv == 0) {
		uint64_t offset = emulated_offset + (sector * SECTOR_SIZE);
		size_t size = count * SECTOR_SIZE;

		seek_64bit(emulated_drive, offset);
		fread(buff, size, 1, emulated_drive);

		return RES_OK;
	}
	else {
		return STA_NOINIT;
	}
}
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
	if (pdrv == 0) {
		uint64_t offset = emulated_offset + (sector * SECTOR_SIZE);
		size_t size = count * SECTOR_SIZE;

		seek_64bit(emulated_drive, offset);
		fwrite(buff, size, 1, emulated_drive);
		return RES_OK;
	}
	else {
		return STA_NOINIT;
	}
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
	if (pdrv == 0) {
		switch (cmd) {
		case GET_SECTOR_SIZE:
			*(DWORD*)buff = SECTOR_SIZE;
			return RES_OK;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = (DWORD)emulated_size / SECTOR_SIZE;
			return RES_OK;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 1;
			return RES_OK;
		case CTRL_SYNC:
			return RES_OK;
		}
		return RES_PARERR;
	}

	return RES_PARERR;
}

uint32_t get_fattime(void) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	return ((tm.tm_year - 1980) << 25) | (tm.tm_mon << 21) | (tm.tm_mday << 16) | (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_min >> 1);
}