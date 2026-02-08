#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib/compat.h"
#include "lib/path.h"
#include "lib/vci.h"
#include "lib/psv.h"
#include "lib/mbr.h"
#include "lib/exfat.h"
#include "lib/gcauthmgr.h"
#include "lib/npdrm.h"

#include <time.h>
#include <assert.h>

int read_title_id(char* title_id) {
	char scratch[0x1028] = { 0 };
	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	if (f_opendir(&dr, "/app") == FR_OK) {
		if (f_readdir(&dr, &fileinfo) == FR_OK) {
			assert(fileinfo.fname[0] != '\0');
			strncpy(title_id, fileinfo.fname[0], 10);
			f_closedir(&dr);
		}
	}
}

int read_license(SceNpDrmLicense* license) {
	char scratch[0x1028] = { 0 };
	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	read_title_id(scratch);

	snprintf(scratch, sizeof(scratch) - 1, "/license/app/%s", scratch);
	if (f_opendir(&dr, scratch) == FR_OK) {
		if (f_readdir(&dr, &fileinfo) == FR_OK) {
			assert(fileinfo.fname[0] != '\0');
			snprintf(scratch, sizeof(scratch) - 1, "%s/%s", scratch, fileinfo.fname);
			f_closedir(&dr);

			if (f_open(&fl, scratch, FA_READ) == FR_OK) {
				uint32_t rd = 0;
				f_read(&fl, license, sizeof(SceNpDrmLicense), &rd);
				f_close(&fl);

				return 1;
			}

		}
	}

		return 0;
	}

	if (f_findfirst(&dr, &fileinfo, "/license/app", "*.rif") == FR_OK) {
		assert(fileinfo.fname != '\0');

		if (f_open(&fl, fileinfo.fname, FA_READ) == FR_OK) {
			if (f_read(&fl, &license, sizeof(license), NULL) == FR_OK) {
				f_close(&fl);
				return FR_OK;
			}
		}


	}
	f_closedir(&dr);
	return FR_INVALID_OBJECT;
}


int main(int argc, char** argv) {
	int ret = 0;
	size_t rd, total = 0;
	char gc_img[0x500] = { 0 };


	if (argc >= 2) {
		strncpy(gc_img, argv[1], sizeof(gc_img) - 1);
	}
	else {
		fprintf(stderr, "Usage: gc2nonpdrm <gc_img>\n");
		return -1;
	}

	if (file_exists(gc_img)) {

		FILE* img_file = fopen(gc_img, "rb");
		
		uint8_t sector[SECTOR_SIZE] = { 0 };
		fread(sector, 1, sizeof(sector), img_file);

		PsvHeader* psv_header = (PsvHeader*)sector;
		VciHeader* vci_header = (VciHeader*)sector;

		uint8_t rif_key[0x20]	= { 0 };
		uint8_t rif_hash[0x14]  = { 0 };
		uint8_t klicensee[0x20] = { 0 };

		SceNpDrmLicense license;

		if (memcmp(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic)) == 0 && psv_header->version == PSV_VER) {
			// read rif key
			memcpy(rif_key, psv_header->cart_secret, sizeof(psv_header->cart_secret));
			memcpy(rif_hash, psv_header->cart_hash, sizeof(psv_header->cart_hash));

			fseek(img_file, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);
			fread(sector, 1, sizeof(sector), img_file);
			
		}
		else if (memcmp(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic)) == 0 && vci_header->major_version == VCI_MAJOR_VER) {
			// generate rif key
			get_cart_secret(&vci_header->keys, rif_key);
			get_cart_hash(&vci_header->keys, rif_hash);

			fseek(img_file, sizeof(VciHeader), SEEK_SET);
			fread(sector, 1, sizeof(sector), img_file);

		}
		
		uint64_t mbr_start = ftell(img_file) - SECTOR_SIZE;

		SceMbr* mbr = (SceMbr*)sector;
		for (int i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
			if (mbr->partitions[i].code == ScePartitionCode_EMPTY) continue;
			if (mbr->partitions[i].type == ScePartitionType_EXFAT) {
				uint64_t offset = mbr_start + (mbr->partitions[i].offset * SECTOR_SIZE);
				uint64_t size = mbr->partitions[i].size * SECTOR_SIZE;

				FATFS fs;
				ff_init(img_file, offset, size);
				if (f_mount(&fs, "0:", 1) == FR_OK) {
					read_recursive("/app", "app");
					if (read_license(&license)) {
						decrypt_klicensee(rif_key, klicensee, &license);
					}

				}

				f_unmount("0:");
			}
		}


	}

}
