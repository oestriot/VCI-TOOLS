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

#include "ff/ff_port.h"
#include "ff/ff.h"
#include <time.h>
#include <assert.h>

static uint8_t buffer[SECTOR_SIZE * 0x5000];


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
		if (memcmp(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic)) == 0 && psv_header->version == PSV_VER) {
			fseek(img_file, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);
			fread(sector, 1, sizeof(sector), img_file);
		}
		else if (memcmp(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic)) == 0 && vci_header->major_version == VCI_MAJOR_VER) {
			fseek(img_file, sizeof(VciHeader), SEEK_SET);
			fread(sector, 1, sizeof(sector), img_file);
		}
		
		uint64_t mbr_start = ftell(img_file) - SECTOR_SIZE;

		SceMbr* mbr = (SceMbr*)sector;
		for (int i = 0; i < (sizeof(mbr->partitions) / sizeof(ScePartition)); i++) {
			if (mbr->partitions[i].code == ScePartitionCode_EMPTY) continue;
			if (mbr->partitions[i].type == ScePartitionType_EXFAT) {
				ff_init(img_file, mbr_start + (mbr->partitions[i].offset * SECTOR_SIZE));

				FATFS fs;
				f_mount(&fs, "", 0);
				//read_recursive("", "gro0");
			}
		}


	}

}
