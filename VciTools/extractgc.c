#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "lib/compat.h"
#include "lib/path.h"
#include "lib/vci.h"
#include "lib/exfat.h"
#include "lib/psv.h"
#include "lib/mbr.h"
#include "lib/gcauthmgr.h"

static uint8_t sector[SECTOR_SIZE];
static uint8_t buffer[SECTOR_SIZE * 0x5000];

void dump_partiton(FILE* img, ScePartition* part, uint64_t mbr_start_pos, const char* output_file) {
	FILE* partition = fopen(output_file, "wb");
	uint64_t total = 0;

	uint64_t real_part_offset = mbr_start_pos + (part->offset * SECTOR_SIZE);
	uint64_t real_part_size = (part->size * SECTOR_SIZE);

	fseek(img, real_part_offset, SEEK_SET);

	do {
		size_t rd = fread(buffer, 1, (size_t)(((total + sizeof(buffer)) <= real_part_size) ? sizeof(buffer) : real_part_size - total), img);
		total += fwrite(buffer, 1, rd, partition);
		printf("%s: %llu / %llu (%.0f%%)\r", output_file, total, real_part_size, (((double)total / (double)real_part_size) * 100.0));
	} while (total < real_part_size);

	fclose(partition);
	printf("\n");
}


int main(int argc, char** argv) {
	int ret = 0;
	uint8_t dump_raw = 0;

	FATFS fs = { 0 };
	char scratch[0x500] = { 0 };
	char gc_img[0x500] = { 0 };
	char out_file[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(gc_img, argv[1], sizeof(gc_img) - 1);
	}
	else {
		fprintf(stderr, "Usage: extractgc <gc_image> [dump_raw]\n");
		fprintf(stderr, "gc_image\tpath to a .img, .psv, or .vci file.\n");
		fprintf(stderr, "dump_raw\t'true' or 'false'\tdetermines wether to extract disk images, or extract files\n");

		return -1;
	}
	
	if (argc >= 3) {
		dump_raw = (strcmp(argv[2], "true") == 0);
	}

	if (file_exists(gc_img)) {
		FILE* img = fopen(gc_img, "rb");

		fread(sector, 1, sizeof(sector), img);

		VciHeader* vci = (VciHeader*)sector;
		
		if (memcmp(vci->magic, VCI_MAGIC, sizeof(vci->magic)) == 0 && vci->major_version == VCI_MAJOR_VER) {
			printf("Detected: vita cartridge image\n");
			
			// write keys
			change_extension("-keys.bin", gc_img, sizeof(gc_img), scratch);
			write_file(scratch, &vci->keys, sizeof(GcCmd56Keys));
			
		}
		PsvHeader* psv = (PsvHeader*)sector;
		if (memcmp(psv->magic, PSV_MAGIC, sizeof(psv->magic)) == 0 && psv->version == PSV_VER) {
			printf("Detected: psvgamesd\n");
			
			// write cart secret
			change_extension("-cart_secret.bin", gc_img, sizeof(gc_img), scratch);
			write_file(scratch, psv->cart_secret, sizeof(psv->cart_secret));

			// write keys
			change_extension("-cart_hash.bin", gc_img, sizeof(gc_img), scratch);
			write_file(scratch, psv->cart_hash, sizeof(psv->cart_hash));

			// seek to MBR
			fseek(img, psv->image_offset * SECTOR_SIZE, SEEK_SET);
			
		}
		
		// Read MBR
		uint64_t mbr_start_pos = ftell(img);
		fread(sector, 1, sizeof(sector), img);
		SceMbr* mbr = (SceMbr*)sector;

		if (memcmp(mbr->magic, SCE_MBR_MAGIC, sizeof(mbr->magic)) == 0) {
			for (size_t i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
				if (mbr->partitions[i].code == ScePartitionCode_EMPTY) continue;
								

				if (!dump_raw && (mbr->partitions[i].type == ScePartitionType_EXFAT || mbr->partitions[i].type == ScePartitionType_FAT16)) {
					uint64_t offset = mbr_start_pos + (mbr->partitions[i].offset * SECTOR_SIZE);
					uint64_t size = mbr->partitions[i].size * SECTOR_SIZE;

					change_extension("", gc_img, sizeof(gc_img), scratch);
					snprintf(out_file, sizeof(out_file) - 1, "%s/%s", scratch, partition_code_to_name(mbr->partitions[i].code));

					ff_init(img, offset, size);
					if (f_mount(&fs, "0:", 1) == FR_OK) {
						read_recursive("/", out_file);
						f_unmount("0:");
					}
					else {
						fprintf(stderr, "image file contains an invalid exFAT partition!!!\n");
						ret = -1;
					}
				}
				else {
					// get partition filename 
					char extension[0x30] = { 0 };
					snprintf(extension, sizeof(extension) - 1, ".%s-%s.img", partition_code_to_name(mbr->partitions[i].code), format_id_to_name(mbr->partitions[i].type));
					change_extension(extension, gc_img, sizeof(gc_img), out_file);

					dump_partiton(img, &mbr->partitions[i], mbr_start_pos, out_file);
				}

			}
		}
		else {
			uint64_t size = get_filesize(img);
			change_extension("", gc_img, sizeof(gc_img), out_file);

			ff_init(img, 0, size);

			if (f_mount(&fs, "0:", 1) == FR_OK) {
				read_recursive("/", out_file);
				f_unmount("0:");
			}
			else {
				fprintf(stderr, "image file has an invalid SceMbr and is also not itself a valid exFAT partition!!!\n");
				ret = -1;

			}

		}
		

		fclose(img);
		return ret;
	}
	else {
		fprintf(stderr, "%s: file not found.\n", gc_img);
		ret = -1;
	}
	return ret;

}
