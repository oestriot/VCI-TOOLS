#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include "lib/compat.h"
#include "lib/vci.h"
#include "lib/psv.h"
#include "lib/mbr.h"
#include "lib/sha1.h"
#include "lib/sha256.h"
#include "lib/path.h"
#include "lib/gcauthmgr.h"

static uint8_t buffer[SECTOR_SIZE * 0x5000];

static void create_psv_header(const VciHeader* vci_header, PsvHeader* psv_header) {
	memset(psv_header, 0, sizeof(PsvHeader));
	memcpy(psv_header->magic, PSV_MAGIC, sizeof(psv_header->magic));
	psv_header->version = PSV_VER;
	psv_header->flags = PSV_FLAGS;
	
	get_cart_secret(&vci_header->keys, psv_header->cart_secret);
	get_cart_hash(&vci_header->keys, psv_header->cart_hash);

	psv_header->image_offset = 1;
	psv_header->image_size = vci_header->device_size;
}


int main(int argc, char** argv) {

	int ret = 0;
	size_t rd = 0;
	uint64_t total = 0;

	char vci_file[0x500] = { 0 };
	char psv_file[0x500] = { 0 };
	char key_file[0x500] = { 0 };

	if (argc >= 2) {
		strncpy(vci_file, argv[1], sizeof(vci_file) - 1);
	}
	else {
		fprintf(stderr, "Usage: vci2psv <vci_file> [psv_file] [keys_file]\n");
		return -1;
	}

	if (argc >= 3) {
		strncpy(psv_file, argv[2], sizeof(psv_file) - 1);
	}
	else {
		change_extension(".psv", vci_file, sizeof(vci_file), psv_file);
	}
	
	if (argc >= 4) {
		strncpy(key_file, argv[3], sizeof(key_file) - 1);
	}
	else {
		change_extension("-keys.bin", vci_file, sizeof(vci_file), key_file);
	}

	if (file_exists(vci_file)) {
		FILE* vci = fopen(vci_file, "rb");
		uint64_t vci_size = get_filesize(vci);

		VciHeader vci_header;
		PsvHeader psv_header;

		// read vci header ..
		fread(&vci_header, 1, sizeof(VciHeader), vci);

		if (memcmp(vci_header.magic, VCI_MAGIC, sizeof(vci_header.magic)) == 0 && vci_header.major_version == VCI_MAJOR_VER) {
			// create psv file
			FILE* psv = fopen(psv_file, "wb");

			// create psv header and write to psv
			create_psv_header(&vci_header, &psv_header);
			total += fwrite(&psv_header, 1, sizeof(PsvHeader), psv);

			// write the keys file ..
			write_file(key_file, &vci_header.keys, sizeof(GcCmd56Keys));

			// initalize sha256 ...
			Sha256Context sha256;
			Sha256Initialise(&sha256);

			// read & hash all sectors
			do {
				rd = fread(buffer, 1, sizeof(buffer), vci);
				Sha256Update(&sha256, buffer, sizeof(buffer));
				total += fwrite(buffer, 1, rd, psv);

				printf("%s: %llu / %llu (%.0f%%)\r", psv_file, total, vci_size, (((double)total / (double)vci_size) * 100.0));
			} while (rd == sizeof(buffer));

			// update the hash in the psv header
			Sha256Finalise(&sha256, psv_header.all_sectors_sha256);

			// write modified header to disk
			fseek(psv, 0, SEEK_SET);
			fwrite(&psv_header, 1, sizeof(PsvHeader), psv);

			// close psv file
			fclose(psv);
		}
		else {
			fprintf(stderr, "Invalid vci magic or wrong version!\n");
			ret = -1;
		}
		fclose(vci);
		return ret;
	}
	else {
		fprintf(stderr, "%s: file not found.\n", vci_file);
		return -1;
	}

}