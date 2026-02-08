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

static char scratch[0x1028] = { 0 };
static uint8_t sector[SECTOR_SIZE] = { 0 };
static uint8_t buffer[SECTOR_SIZE * 0x5000] = { 0 };

int dump_partiton(FILE* img, ScePartition* part, uint64_t mbr_start_pos, const char* output_file) {

	FILE* partition = fopen(output_file, "wb");
	if (partition != NULL) {
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
		return 1;
	}

	return 0;
}



int main(int argc, char** argv) {
	int res = -1;
	uint8_t dump_raw = 0;

	FATFS fs = { 0 };
	char gc_img[0x500] = { 0 };
	char output_folder[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(gc_img, argv[1], sizeof(gc_img) - 1);
	}
	else {
		fprintf(stderr, "Usage: extractgc <gc_image> [dump_raw] [out_file]\n");
		fprintf(stderr, "gc_image\tpath to a .img, .psv, or .vci file.\n");
		fprintf(stderr, "dump_raw\t'true' or 'false'\tdetermines wether to extract disk images, or extract files\n");

		return res;
	}
	
	if (argc >= 3) {
		dump_raw = (strcmp(argv[2], "true") == 0);
	}

	if (argc >= 4) {
		strncpy(output_folder, argv[3], sizeof(output_folder) - 1);
	}
	else {
		change_extension("", gc_img, sizeof(gc_img), output_folder);
	}

	if (file_exists(gc_img)) {
		FILE* img = fopen(gc_img, "rb");
		fread(sector, 1, sizeof(sector), img);

		PsvHeader* psv = (PsvHeader*)sector;
		VciHeader* vci = (VciHeader*)sector;
		SceMbr* mbr = (SceMbr*)sector;

		// check for vci header ..
		if (memcmp(vci->magic, VCI_MAGIC, sizeof(vci->magic)) == 0 && vci->major_version == VCI_MAJOR_VER) { // gctoolkit
			printf("Detected: vita cartridge image (VCI)\n");
			
			// write keys
			snprintf(scratch, sizeof(scratch) - 1, "%s/gc_cmd56_keys.bin", output_folder);
			write_file(scratch, &vci->keys, sizeof(GcCmd56Keys));

			// seek to MBR
			fseek(img, sizeof(VciHeader), SEEK_SET);
		}
		if (memcmp(psv->magic, PSV_MAGIC, sizeof(psv->magic)) == 0 && psv->version == PSV_VER) { // psvgamesd
			printf("Detected: psvgamesd (PSV)\n");
			
			// write cart secret
			snprintf(scratch, sizeof(scratch) - 1, "%s/cart-secret.bin", output_folder);
			write_file(scratch, psv->cart_secret, sizeof(psv->cart_secret));

			// write keys
			snprintf(scratch, sizeof(scratch) - 1, "%s/cart-hash.bin", output_folder);
			write_file(scratch, psv->cart_hash, sizeof(psv->cart_hash));

			// seek to MBR
			fseek(img, psv->image_offset * SECTOR_SIZE, SEEK_SET);
			
		}
		if (memcmp(mbr->magic, SCE_MBR_MAGIC, sizeof(mbr->magic)) == 0) { // raw img
			printf("Detected: raw (img)\n");

			// seek to MBR
			fseek(img, 0, SEEK_SET);
		}
		
		// Read MBR
		uint64_t mbr_start_pos = ftell(img);
		fread(sector, 1, sizeof(sector), img);

		if (memcmp(mbr->magic, SCE_MBR_MAGIC, sizeof(mbr->magic)) == 0) {
			create_directories(output_folder, 0);

			for (size_t i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
				if (mbr->partitions[i].code == ScePartitionCode_EMPTY) continue;			
				const char* partition_name = partition_code_to_name(mbr->partitions[i].code);
				const char* format_name = format_id_to_name(mbr->partitions[i].type);
				const char* modifiers = mbr->partitions[i].active ? "" : "-ina";

				if (!dump_raw && (mbr->partitions[i].type == ScePartitionType_EXFAT || mbr->partitions[i].type == ScePartitionType_FAT16)) {
					uint64_t offset = mbr_start_pos + (mbr->partitions[i].offset * SECTOR_SIZE);
					uint64_t size = mbr->partitions[i].size * SECTOR_SIZE;

					// get output filename
					snprintf(scratch, sizeof(scratch) - 1, "%s/%s%s", output_folder, partition_name, modifiers);

					// extract exfat image
					ff_init(img, offset, size);
					if (f_mount(&fs, "0:", 1) == FR_OK) {
						
						if (read_recursive("", scratch)) {
							res = 0;
						}
						else {
							fprintf(stderr, "warn: failed to read one or more files in the %s: exFAT partition.\n", partition_name);
							break;
						}

						f_unmount("0:");
					}
					else {
						fprintf(stderr, "warn: expected %s: to contain a exFAT partition; but it is missing or corrupted..\n", partition_name);
						goto dump;
					}
				}
				else {
dump:
					// get partition filename 
					snprintf(scratch, sizeof(scratch) - 1, "%s/%s%s-%s.img", output_folder, partition_name, modifiers, format_name);

					// dump partition ..
					if (!dump_partiton(img, &mbr->partitions[i], mbr_start_pos, scratch)) {
						fprintf(stderr, "err: failed to dump partition: %s\n", partition_name);
						break;
					}
				}

			}
		}
		else {
			uint64_t size = get_filesize(img);
			change_extension("", gc_img, sizeof(gc_img), output_folder);

			ff_init(img, 0, size);

			if (f_mount(&fs, "0:", 1) == FR_OK) {
				if (read_recursive("/", output_folder)) {
					res = 0;
				}
				else {
					fprintf(stderr, "err: failed to read one or more files from exFAT partition.\n");
				}

				f_unmount("0:");
			}
			else {
				fprintf(stderr, "err: %s: could not find a valid SceMBR, but also the file is not a valid exFAT partition\n", gc_img);
			}

		}
		

		fclose(img);
		return res;
	}
	else {
		fprintf(stderr, "err: %s: file not found.\n", gc_img);
	}
	return res;

}
