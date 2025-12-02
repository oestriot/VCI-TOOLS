#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include "compat.h"

#include "vci.h"
#include "psv.h"
#include "mbr.h"

#include "path.h"

static uint8_t buffer[SECTOR_SIZE * 0x5000];

void dump_image(const FILE* gc_img, const char* img_path, const uint64_t image_size) {
	uint64_t rd, total = 0;
	FILE* img = fopen(img_path, "wb");

	do {
		uint64_t to_read = (((total + sizeof(buffer)) <= image_size) ? sizeof(buffer) : image_size - total);
		uint64_t rd = fread(buffer, 1, to_read, gc_img);

		if (rd < to_read) memset((buffer + rd), 0x00, (to_read - rd));

		total += fwrite(buffer, 1, to_read, img);
		printf("%s: %llu / %llu (%.0f%%)\r", img_path, total, image_size, (((double)total / (double)image_size) * 100.0));
	} while (total < image_size);

	fclose(img);
}


int main(int argc, char** argv) {
	int ret = 0;

	char gc_img_file[0x500] = { 0 };
	char img_file[0x500] = { 0 };
	char scratch[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(gc_img_file, argv[1], sizeof(gc_img_file) - 1);
	}
	else {
		fprintf(stderr, "Usage: vci2psv <gc_img_file> [img_file]\n");
		return -1;
	}

	if (argc >= 3) {
		strncpy(img_file, argv[2], sizeof(img_file) - 1);
	}
	else {
		change_extension(".img", gc_img_file, sizeof(gc_img_file), img_file);
	}


	if (file_exists(gc_img_file)) {
		FILE* gc_img = fopen(gc_img_file, "rb");

		uint8_t sector[SECTOR_SIZE];
		fread(sector, 1, sizeof(sector), gc_img);

		// read header ..
		VciHeader* vci_header = (VciHeader*)sector;
		PsvHeader* psv_header = (PsvHeader*)sector;

		if (memcmp(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic)) == 0 && vci_header->major_version == VCI_MAJOR_VER) {
			
			// write the keys.bin file ..
			change_extension("-keys.bin", gc_img_file, sizeof(gc_img_file), scratch);
			write_file(scratch, &vci_header->keys, sizeof(GcCmd56Keys));

			dump_image(gc_img, img_file, vci_header->device_size);

		}
		else if (memcmp(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic)) == 0 && psv_header->version == PSV_VER) {
			
			// write out cart secret and cart hash ..

			change_extension("-cart_secret.bin", gc_img_file, sizeof(gc_img_file), scratch);
			write_file(scratch, psv_header->cart_secret, sizeof(psv_header->cart_secret));

			change_extension("-cart_hash.bin", gc_img_file, sizeof(gc_img_file), scratch);
			write_file(scratch, psv_header->cart_hash, sizeof(psv_header->cart_hash));

			// seek to image start
			fseek(gc_img, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);

			dump_image(gc_img, img_file, psv_header->image_size);
		}
		else {
			fprintf(stderr, "Invalid magic or wrong version!\n");
			ret = -1;
		}

		fclose(gc_img);
		return ret;
	}
	else {
		fprintf(stderr, "%s: file not found.\n", gc_img_file);
		return -1;
	}

}