#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "lib/compat.h"
#include "lib/path.h"
#include "lib/vci.h"
#include "lib/psv.h"
#include "lib/mbr.h"
#include "lib/gcauthmgr.h"

#include "ff/ff_port.h"
#include "ff/ff.h"

static uint8_t buffer[SECTOR_SIZE * 0x5000];
static FATFS fs;

void read_recursive(char* prev, const char* extract_dir) {
	char path[MAX_PATH] = { 0 };
	char opath[MAX_PATH] = { 0 };

	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	FRESULT res = f_opendir(&dr, prev);
	if (res == FR_OK) {
		while(1) {
			res = f_readdir(&dr, &fileinfo);
			if (res != FR_OK || fileinfo.fname[0] == '\0') break;

			//size_t prev_len = strlen(prev);
			if (strcmp(prev, "/") == 0) prev++;

			snprintf(path, sizeof(path) - 1, "%s/%s", prev, fileinfo.fname);
			snprintf(opath, sizeof(opath) - 1, "%s%s/%s", extract_dir, prev, fileinfo.fname);
			
			if ((fileinfo.fattrib & AM_DIR) != 0) {
				create_directories(opath, 0);
				read_recursive(path, extract_dir);
			}
			if ((fileinfo.fattrib & AM_DIR) == 0) {
				create_directories(opath, 0);
				f_open(&fl, path, FA_READ);
				FILE* out_fd = fopen(opath, "wb");

				if (out_fd != NULL) {
					uint32_t rd = 0;
					uint64_t total = 0;

					do {
						f_read(&fl, buffer, sizeof(buffer), &rd);
						total += fwrite(buffer, 1, rd, out_fd);
						printf("%s: %llu / %llu (%.0f%%)\r", opath, total, fileinfo.fsize, (((double)total / (double)fileinfo.fsize) * 100.0));
					} while (total < fileinfo.fsize);
					printf("\n");

					fclose(out_fd);
					f_close(&fl);
				}

			}
		}

		f_closedir(&dr);
	}


}

const char* format_id_to_name(int format_id) {
	switch (format_id) {
	case ScePartitionType_EXFAT:
		return "exfat";
	case ScePartitionType_FAT16:
		return "fat16";
	case ScePartitionType_RAW:
		return "raw";
	}
}

const char* partition_code_to_name(int partition_code) {
	switch (partition_code) {
	case ScePartitionCode_EMPTY:
		return "empty0";
	case ScePartitionCode_IDSTORAGE:
		return "idstorage0";
	case ScePartitionCode_SLB2:
		return "slb2";
	case ScePartitionCode_OS0:
		return "os0";
	case ScePartitionCode_VS0:
		return "vs0";
	case ScePartitionCode_VD0:
		return "vd0";
	case ScePartitionCode_TM0:
		return "tm0";
	case ScePartitionCode_UR0:
		return "ur0";
	case ScePartitionCode_UX0:
		return "ux0";
	case ScePartitionCode_GRO0:
		return "gro0";
	case ScePartitionCode_GRW0:
		return "grw0";
	case ScePartitionCode_UD0:
		return "ud0";
	case ScePartitionCode_SA0:
		return "sa0";
	case ScePartitionCode_MEDIAID:
		return "mediaid0";
	case ScePartitionCode_PD0:
		return "pd0";
	case ScePartitionCode_UNUSED:
		return "unused0";
	default:
		return "unknown0";
	}
}

void dump_partiton(FILE* img, ScePartition* part, uint64_t mbr_start_pos, const char* output_file) {
	FILE* partition = fopen(output_file, "wb");

	uint64_t total = 0;
	
	uint64_t real_part_offset = mbr_start_pos + (part->offset * SECTOR_SIZE);
	uint64_t real_part_size	  = (part->size * SECTOR_SIZE);

	fseek(img, real_part_offset, SEEK_SET);

	do {
		uint64_t rd = fread(buffer, 1, (((total + sizeof(buffer)) <= real_part_size) ? sizeof(buffer) : real_part_size - total), img);
		total += fwrite(buffer, 1, rd, partition);
		printf("%s: %llu / %llu (%.0f%%)\r", output_file, total, real_part_size, (((double)total / (double)real_part_size) * 100.0));
	} while (total < real_part_size);

	fclose(partition);
	printf("\n");
}

int main(int argc, char** argv) {
	int ret = 0;
	uint8_t dump_raw = 0;
	size_t rd, total = 0;

	char scratch[0x500] = { 0 };
	char gc_img[0x500] = { 0 };

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

		uint8_t sector[SECTOR_SIZE];
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
				const char* partition_name = partition_code_to_name(mbr->partitions[i].code);

				printf("Parsing ... %s\n", partition_name);

				// get partition filename 
				char extension[0x30] = { 0 };
				snprintf(extension, sizeof(extension) - 1, ".%s-%s.img", partition_name, format_id_to_name(mbr->partitions[i].type));
				change_extension(extension, gc_img, sizeof(gc_img), scratch);
				
				if (!dump_raw && (mbr->partitions[i].type == ScePartitionType_EXFAT || mbr->partitions[i].type == ScePartitionType_FAT16)) {
					uint64_t offset = mbr_start_pos + (mbr->partitions[i].offset * SECTOR_SIZE);
					ff_init(img, offset);
					f_mount(&fs, "0:", 1);
					read_recursive("/", partition_name);
					f_unmount("0:");
				}
				else {
					dump_partiton(img, &mbr->partitions[i], mbr_start_pos, scratch);
				}

			}
		}
		else {
			fprintf(stderr, "image file has an invalid SceMbr !!!\n");
			ret = -1;
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
