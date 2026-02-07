#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "lib/compat.h"
#include "lib/vci.h"
#include "lib/psv.h"
#include "lib/mbr.h"
#include "lib/sha1.h"
#include "lib/sha256.h"
#include "lib/path.h"
#include "lib/gcauthmgr.h"

static uint8_t buffer[SECTOR_SIZE * 0x5000];

static void create_vci_header(const uint64_t size, VciHeader* vci_header) {
	memset(vci_header, 0, sizeof(VciHeader));
	memcpy(vci_header->magic, VCI_MAGIC, sizeof(vci_header->magic));
	vci_header->major_version = VCI_MAJOR_VER;
	vci_header->minor_version = VCI_MINOR_VER;
	vci_header->device_size = size;
}

static int verify_keys(const uint8_t* cart_secret, GcCmd56Keys* gc_keys) {

	uint8_t got_cart_secret[0x20];
	get_cart_secret(gc_keys, got_cart_secret);
	return (memcmp(got_cart_secret, cart_secret, sizeof(got_cart_secret)) == 0);
}

void write_vci(FILE* img, const char* vci_file, const VciHeader* header, const uint64_t img_sz) {
	uint64_t rd, total = 0;
	FILE* vci = fopen(vci_file, "wb");

	// write the VCI header ...
	total += fwrite(header, 1, sizeof(VciHeader), vci);

	do {
		rd = fread(buffer, 1, sizeof(buffer), img);
		total += fwrite(buffer, 1, rd, vci);

		printf("%s: %llu / %llu (%.0f%%)\r", vci_file, total, img_sz, (((double)total / (double)img_sz) * 100.0));
	} while (rd == sizeof(buffer));

	fclose(vci);
}

int main(int argc, char** argv) {
	int ret = 0;

	char img_file[0x500] = { 0 };
	char key_file[0x500] = { 0 };
	char vci_file[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(img_file, argv[1], sizeof(img_file) - 1);
	}
	else {
		fprintf(stderr, "Usage: gc2vci <psv_or_img_file> <keys_file> [vci_file]\n");
		return -1;
	}

	if (argc >= 3) {
		strncpy(key_file, argv[2], sizeof(key_file) - 1);
	}
	else {
		change_extension("-keys.bin", img_file, sizeof(img_file), key_file);
	}

	if (argc >= 4) {
		strncpy(vci_file, argv[3], sizeof(vci_file) - 1);
	}
	else {
		change_extension(".vci", img_file, sizeof(img_file), vci_file);
	}

	if (file_exists(img_file) && file_exists(key_file)) {
		FILE* img = fopen(img_file, "rb");

		uint8_t sector[SECTOR_SIZE] = { 0 };
		VciHeader vci_header = { 0 };

		// read psv header ..
		fread(sector, SECTOR_SIZE, 1, img);
		uint64_t img_size = get_filesize(img);
		
		PsvHeader* psv_header = (PsvHeader*)sector;
		SceMbr* mbr = (SceMbr*)sector;

		if (memcmp(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic)) == 0 && psv_header->version == PSV_VER) { // is PSV
			// create vci header ..
			create_vci_header(psv_header->image_size, &vci_header);

			// read in keys from the keys file
			read_file(key_file, &vci_header.keys, sizeof(VciHeader));

			// validate the keys we just read ... 
			if (verify_keys(psv_header->cart_secret, &vci_header.keys)) {
				fseek(img, psv_header->image_offset * SECTOR_SIZE, SEEK_SET);

				// write the vci file
				write_vci(img, vci_file, &vci_header, img_size);
			}
			else {
				ferror(stderr, "This key file does not match this psv file!");
				ret = -1;
			}
		}
		else if (memcmp(mbr->magic, SCE_MBR_MAGIC, sizeof(mbr->magic)) == 0) { // is SceMbr // raw gamecart image ..

			// create vci header ..
			create_vci_header(img_size, &vci_header);

			// read in keys from the keys file
			read_file(key_file, &vci_header.keys, sizeof(VciHeader));
			
			// seek to image start ..
			fseek(img, 0, SEEK_SET);

			// write the vci file
			write_vci(img, vci_file, &vci_header, img_size + SECTOR_SIZE);
		}
		else {
			fprintf(stderr, "%s was in the wrong format\n", img_file);
			ret = -1;
		}

		fclose(img);
		return ret;
	}
	else {
		fprintf(stderr, "either %s or %s: was not found.\n", img_file, key_file);
		return -1;
	}

}