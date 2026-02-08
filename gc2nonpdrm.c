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
static char scratch[0x1028] = { 0 };


int read_title_id(char* title_id) {
	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	if (f_opendir(&dr, "/app") == FR_OK) {
		if (f_readdir(&dr, &fileinfo) == FR_OK) {
			assert(fileinfo.fname[0] != '\0');
			strncpy(title_id, fileinfo.fname, 9);
			f_closedir(&dr);
			return 1;
		}
	}
	return 0;
}

int write_license(SceNpDrmLicense* old_license, uint8_t* klicensee, const char* out_folder) {
	char title_id[0x10] = { 0 };
	read_title_id(title_id);

	SceNpDrmLicense new_license = { 0 };

	new_license.account_id = 0x0123456789ABCDEFLL;
	new_license.version = swap16(1);
	new_license.version_flags = swap16(1);
	new_license.license_type = swap16(1);

	new_license.license_flags = swap16(swap16(old_license->license_flags) & ~0x400);
	new_license.flags = old_license->flags;

	if (swap32(old_license->sku_flag) == 1 || swap32(old_license->sku_flag) == 3) {
		new_license.sku_flag = swap32(3);
	}
	else {
		new_license.sku_flag = swap32(0);
	}

	memcpy(new_license.content_id, old_license->content_id, sizeof(old_license->content_id));
	memcpy(new_license.key1, klicensee, sizeof(new_license.key1));

	snprintf(scratch, sizeof(scratch) - 1, "%s/app/%s/sce_sys/package/work.bin", out_folder, title_id);
	write_file(scratch, &new_license, sizeof(SceNpDrmLicense));
	return 1;
}

int read_license(SceNpDrmLicense* license) {
	char title_id[0x10] = { 0 };
	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	read_title_id(title_id);

	snprintf(scratch, sizeof(scratch) - 1, "/license/app/%s", title_id);
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




int main(int argc, char** argv) {
	int res = -1;
	char gc_img[0x500] = { 0 };
	char out_folder[0x500] = { 0 };


	if (argc >= 2) {
		strncpy(gc_img, argv[1], sizeof(gc_img) - 1);
	}
	else {
		fprintf(stderr, "Usage: gc2nonpdrm <vci or psv> [out folder]\n");
		return -1;
	}

	if (argc >= 3) {
		strncpy(out_folder, argv[2], sizeof(out_folder) - 1);
	}
	else {
		change_extension("", gc_img, sizeof(gc_img), out_folder);
	}

	create_directories(out_folder, 1);

	if (file_exists(gc_img)) {
		FILE* img = fopen(gc_img, "rb");
		
		uint8_t sector[SECTOR_SIZE] = { 0 };
		fread(sector, 1, sizeof(sector), img);

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

			fseek(img, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);
			fread(sector, 1, sizeof(sector), img);
			
		}
		else if (memcmp(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic)) == 0 && vci_header->major_version == VCI_MAJOR_VER) {
			// generate rif key
			get_cart_secret(&vci_header->keys, rif_key);
			get_cart_hash(&vci_header->keys, rif_hash);

			fseek(img, sizeof(VciHeader), SEEK_SET);
			fread(sector, 1, sizeof(sector), img);

		}
		
		uint64_t mbr_start = ftell(img) - SECTOR_SIZE;

		SceMbr* mbr = (SceMbr*)sector;
		for (int i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
			if (mbr->partitions[i].code == ScePartitionCode_GRO0 &&
				mbr->partitions[i].type == ScePartitionType_EXFAT) {
				uint64_t offset = mbr_start + (mbr->partitions[i].offset * SECTOR_SIZE);
				uint64_t size = mbr->partitions[i].size * SECTOR_SIZE;
				

				FATFS fs;
				ff_init(img, offset, size);
				if (f_mount(&fs, "0:", 1) == FR_OK) {
					if (read_recursive("/app", out_folder)) {
						if (read_license(&license)) {
							decrypt_klicensee(rif_key, klicensee, &license);
							if (write_license(&license, klicensee, out_folder)) {
								res = 0;
							}
						}

					}
				}

				f_unmount("0:");
				break;
			}
		}


	}

	return res;

}
