#include "lib/compat.h"
#include "lib/npdrm.h"
#include "lib/path.h"

void hex2bin(char* bytes, size_t len, const char* hexstr)
{
	size_t hexstrLen = strlen(hexstr);
	size_t bytesLen = hexstrLen / 2;

	if (bytesLen <= len) {
		int count = 0;
		const char* pos = hexstr;

		for (count = 0; count < bytesLen; count++) {
			sscanf(pos, "%2hhx", &bytes[count]);
			pos += 2;
		}
	}

}


int log_buffer(char* header, uint8_t* buffer, size_t sz) {
	printf("%s", header);
	for (int i = 0; i < sz; i++) {
		printf("%02X", buffer[i]);
	}
	printf("\n");
}

int is_zeros(uint8_t* buffer, size_t sz) {
	for (int i = 0; i < buffer[i]; i++) {
		if (buffer[i] != 0) return 0;
	}
	return 1;
}

int main(int argc, char** argv) {
	char npdrm_file[0x500] = { 0 };
	char nonpdrm_file[0x500] = { 0 };
	char act_file[0x500] = { 0 };
	char console_id[0x10] = { 0 };
	uint8_t vita = 1;

	if (argc >= 2) {
		strncpy(npdrm_file, argv[1], sizeof(npdrm_file) - 1);
	}
	else {
		fprintf(stderr, "Usage: npdrm2nonpdrm <npdrm_rif> [nonpdrm_rif] [act.dat] [console_id] [psp]\n");
		return -1;
	}


	if (argc >= 3) {
		strncpy(nonpdrm_file, argv[2], sizeof(nonpdrm_file) - 1);
	}
	else {
		change_extension("-nonpdrm.rif", npdrm_file, sizeof(nonpdrm_file), nonpdrm_file);
	}

	if (argc >= 4) {
		strncpy(act_file, argv[3], sizeof(act_file) - 1);
	}

	if (argc >= 5) {
		hex2bin(console_id, sizeof(console_id), argv[4]);
	}

	if (argc >= 6) {
		if (strcmp(argv[5], "yes") == 0) {
			vita = 0;
		}
	}

	if (file_exists(npdrm_file)) {
		SceNpDrmLicense license = { 0 };
		SceNpDrmActivationData activation = { 0 };
		uint8_t klicensee[0x10] = { 0 };

		FILE* npdrm = fopen(npdrm_file, "rb");
		fread(&license, 1, sizeof(license), npdrm);
		fclose(npdrm);

		if (!is_zeros(act_file, sizeof(act_file))) {
			FILE* actdat = fopen(act_file, "rb");
			fread(&activation, 1, sizeof(activation), actdat);
			fclose(actdat);
		}


		init_npdrm(NULL, !is_zeros(console_id, sizeof(console_id)) ? NULL : console_id, !is_zeros(&activation, sizeof(activation)) ? NULL : &activation);
		if (decrypt_klicensee(klicensee, &license, 1)) {
			log_buffer("got klicensee: ", klicensee, KLICENSEE_SIZE);
			if (make_nonpdrm_license(&license, klicensee)) {
				printf("writing %s\n", nonpdrm_file);

				FILE* nonpdrm = fopen(nonpdrm_file, "wb");
				fwrite(&license, 1, sizeof(SceNpDrmLicense), nonpdrm);
				fclose(nonpdrm);
			}
			else {
				fprintf(stderr, "err: failed to create nonpdrm license.\n");
				return -1;
			}
		}
		else {
			fprintf(stderr, "err: failed to decrypt klicensee (no activation record?)\n");
			return -1;
		}

	}
	else {
		fprintf(stderr, "err: no such file: %s\n", npdrm_file);
		return -1;
	}

	return 0;

}