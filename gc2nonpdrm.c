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
static uint8_t sector[SECTOR_SIZE] = { 0 };


#define CREATE_NONPDRM(copy_folder, install_folder) \
	read_title_id(copy_folder, title_id); \
	if (read_recursive(copy_folder, out_folder)) { \
		if (read_license(install_folder, &license, title_id, license_path, sizeof(license_path))) { \
			if (decrypt_klicensee(klicensee, &license, 1)) { \
				if (make_nonpdrm_license(&license, klicensee)) { \
					snprintf(scratch, sizeof(scratch) - 1, "%s/%s/%s/sce_sys/package/work.bin", out_folder, strip_prefix_slash(copy_folder), license_path); \
					write_file(scratch, &license, sizeof(SceNpDrmLicense)); \
				} \
				else { \
					fprintf(stderr, "err: failed to write nonpdrm license file.\n"); \
				} \
			} \
			else { \
				fprintf(stderr, "err: failed to decrypt klicensee.\n"); \
			} \
		} \
		else { \
			fprintf(stderr, "err: failed to read npdrm license file from %s:/license/%s/%s\n", partition_code_to_name(mbr->partitions[i].code), strip_prefix_slash(install_folder), strip_prefix_slash(title_id)); \
		} \
	} else { \
			fprintf(stderr, "err: could not read file in %s:/%s/\n", partition_code_to_name(mbr->partitions[i].code), strip_prefix_slash(copy_folder)); \
	}


int read_title_id(const char* folder, char* title_id) {
	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	if (f_opendir(&dr, folder) == FR_OK) {
		if (f_readdir(&dr, &fileinfo) == FR_OK) {
			assert(fileinfo.fname[0] != '\0');
			strncpy(title_id, fileinfo.fname, 9);
			f_closedir(&dr);
			return 1;
		}
	}
	return 0;
}


int read_license(const char* root, SceNpDrmLicense* license, char* subfolder, char* final_path, size_t sz) {
	char license_path[0x500] = { 0 };

	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };
	uint32_t rd = 0;

	snprintf(license_path, sizeof(license_path) - 1, "/license/%s/%s", strip_prefix_slash(root), subfolder);

	if (f_opendir(&dr, license_path) == FR_OK) {
		if (f_readdir(&dr, &fileinfo) == FR_OK) {
			f_closedir(&dr);

			if ((fileinfo.fattrib & AM_DIR) != 0) {
				snprintf(final_path, sz, "%s/%s", subfolder, fileinfo.fname);
				return read_license(root, license, final_path, final_path, sz);
			}

			if (fileinfo.fname[0] == '\0') return 0;
			snprintf(scratch, sizeof(scratch) - 1, "%s/%s", license_path, fileinfo.fname);

			if (f_open(&fl, scratch, FA_READ) == FR_OK) {
				f_read(&fl, license, sizeof(SceNpDrmLicense), &rd);
				f_close(&fl);

				if (rd < sizeof(SceNpDrmLicense)) return 0;
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
	char license_path[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(gc_img, argv[1], sizeof(gc_img) - 1);
	}
	else {
		fprintf(stderr, "Usage: gc2nonpdrm <vci or psv> [out root]\n");
		return -1;
	}

	if (argc >= 3) {
		strncpy(out_folder, argv[2], sizeof(out_folder) - 1);
	}
	else {
		change_extension("", gc_img, sizeof(gc_img), out_folder);
	}

	if (file_exists(gc_img)) {
		FILE* img = fopen(gc_img, "rb");
		char title_id[0x10] = { 0 };

		fread(sector, 1, sizeof(sector), img);

		PsvHeader* psv_header = (PsvHeader*)sector;
		VciHeader* vci_header = (VciHeader*)sector;

		uint8_t cart_secret[0x20]	= { 0 };
		uint8_t rif_hash[0x14]  = { 0 };
		uint8_t klicensee[0x20] = { 0 };

		SceNpDrmLicense license;

		if (memcmp(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic)) == 0 && psv_header->version == PSV_VER) {
			// read rif key
			memcpy(cart_secret, psv_header->cart_secret, sizeof(psv_header->cart_secret));
			memcpy(rif_hash, psv_header->cart_hash, sizeof(psv_header->cart_hash));

			fseek(img, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);
			fread(sector, 1, sizeof(sector), img);
			
		}
		else if (memcmp(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic)) == 0 && vci_header->major_version == VCI_MAJOR_VER) {
			// generate rif key
			get_cart_secret(&vci_header->keys, cart_secret);
			get_cart_hash(&vci_header->keys, rif_hash);

			fseek(img, sizeof(VciHeader), SEEK_SET);
			fread(sector, 1, sizeof(sector), img);

		}
		
		uint64_t mbr_start = ftell(img) - SECTOR_SIZE;

		SceMbr* mbr = (SceMbr*)sector;
		if (memcmp(mbr->magic, SCE_MBR_MAGIC, sizeof(mbr->magic)) == 0) {
			for (int i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
				if (mbr->partitions[i].code == ScePartitionCode_GRO0 &&
					mbr->partitions[i].type == ScePartitionType_EXFAT) {
					uint64_t offset = mbr_start + (mbr->partitions[i].offset * SECTOR_SIZE);
					uint64_t size = mbr->partitions[i].size * SECTOR_SIZE;
					
					// do not have act.dat or consoleid, so just leave it with only cart_secret;
					init_npdrm(cart_secret, NULL, NULL);

					FATFS fs;
					ff_init(img, offset, size);
					if (f_mount(&fs, "0:", 1) == FR_OK) {
						CREATE_NONPDRM("/app", "app");
						CREATE_NONPDRM("/appinst", "app");
						CREATE_NONPDRM("/acinst", "addcont");

						return 0;
					}
					else {
						fprintf(stderr, "err: %s: has an invalid exFAT partition.\n", partition_code_to_name(mbr->partitions[i].code));
					}

					f_unmount("0:");
					break;
				}
			}
		}
		else {
			fprintf(stderr, "err: %s does not contain a valid SceMbr!!!\n", gc_img);
		}

	}
	else {
		fprintf(stderr, "err: %s: file not found.\n", gc_img);
	}

	return res;

}
