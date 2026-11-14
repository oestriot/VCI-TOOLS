#include <string.h>

#include "lib/compat.h"
#include "lib/npdrm.h"
#include "lib/path.h"
#include "lib/gcauthmgr.h"
#include "lib/log.h"

#define HANDLE_ARG(name, var) if (strcmp(argv[i], "-"name) == 0) { \
									if( (i + 1) >= argc) { return show_usage(); } \
									strncpy(var, argv[i + 1], sizeof(var) - 1); \
								}

static char OPT_NPDRM_FILE[0x500] = { 0 };
static char OPT_NONPDRM_FILE[0x500] = { 0 };
static char OPT_ACT_FILE[0x500] = { 0 };
static char OPT_GC_KEYFILE[0x500] = { 0 };
static char OPT_PLATFORM[0x500] = { 0 };
static char OPT_CONSOLE_ID[0x500] = { 0 };

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


int is_zeros(uint8_t* buffer, size_t sz) {
	for (int i = 0; i < buffer[i]; i++) {
		if(buffer[i] != 0) return 0;
	}
	return 1;
}

int show_usage() {
	fprintf(stderr, "Usage: npdrm2nonpdrm <npdrm_rif>\n");
	fprintf(stderr, "\t -act [act.dat file]\n");
	fprintf(stderr, "\t -gc [cmd56 key file]\n");
	fprintf(stderr, "\t -idps [console id]\n");
	fprintf(stderr, "\t -platform [psp/vita]\n");
	fprintf(stderr, "\t -o [nonpdrm_rif]\n");
	return -1;
}


int main(int argc, char** argv) {
	enable_logging();

	SceNpDrmLicense license = { 0 };
	SceNpDrmActivationData activation = { 0 };
	uint8_t idps[0x10] = { 0 };
	uint8_t cart_secret[0x20] = { 0 };
	uint8_t klicensee[0x10] = { 0 };
	uint8_t is_vita = 1;

	if (argc >= 2) {
		strncpy(OPT_NPDRM_FILE, argv[1], sizeof(OPT_NPDRM_FILE) - 1);
	}
	else {
		return show_usage();
	}


	for (size_t i = 2; i < argc; i++) {
		HANDLE_ARG("act", OPT_ACT_FILE);
		HANDLE_ARG("gc", OPT_GC_KEYFILE);
		HANDLE_ARG("platform", OPT_PLATFORM);
		HANDLE_ARG("idps", OPT_CONSOLE_ID);
		HANDLE_ARG("o", OPT_NONPDRM_FILE);
	}


	if (!is_zeros(OPT_NPDRM_FILE, sizeof(OPT_NPDRM_FILE))) {

		if (!file_read(OPT_NPDRM_FILE, &license, sizeof(SceNpDrmLicense))) {
			fprintf(stderr, "err: cannot read %s", OPT_NPDRM_FILE);
			return -1;
		}

		// read act.dat
		if (!is_zeros(OPT_ACT_FILE, sizeof(OPT_ACT_FILE))) {
			file_read(OPT_ACT_FILE, &activation, sizeof(SceNpDrmActivationData));
		}

		// read gc keyfile
		if (!is_zeros(OPT_GC_KEYFILE, sizeof(OPT_GC_KEYFILE))) {
			GcCmd56Keys keys;
			if (file_read(OPT_GC_KEYFILE, &keys, sizeof(GcCmd56Keys))) {
				get_cart_secret(&keys, cart_secret);
			}
		}

		// read consoleid
		if (!is_zeros(OPT_CONSOLE_ID, sizeof(OPT_CONSOLE_ID))) {
			if (!file_read(OPT_CONSOLE_ID, idps, sizeof(idps))) {
				hex2bin(idps, sizeof(idps), OPT_CONSOLE_ID);
			}
		}

		if (strncmp(OPT_PLATFORM, "psp", sizeof(OPT_PLATFORM) - 1)) {
			is_vita = 0;
		}

		// default arguments
		if (is_zeros(OPT_NONPDRM_FILE, sizeof(OPT_NONPDRM_FILE))) {
			change_extension("-nonpdrm.rif", OPT_NPDRM_FILE, sizeof(OPT_NONPDRM_FILE), OPT_NONPDRM_FILE);
		}

		init_npdrm(
			is_zeros((uint8_t*)cart_secret, sizeof(cart_secret)) ?
				NULL :
				cart_secret,
			is_zeros(idps, sizeof(idps)) ?
				NULL :
				idps,
			is_zeros((uint8_t*)&activation, sizeof(activation)) ?
				NULL :
				&activation
		);
		if (decrypt_klicensee(klicensee, &license, 1)) {
			if (make_nonpdrm_license(&license, klicensee)) {
				printf("Writing %s\n", OPT_NONPDRM_FILE);

				FILE* nonpdrm = fopen(OPT_NONPDRM_FILE, "wb");
				fwrite(&license, 1, sizeof(SceNpDrmLicense), nonpdrm);
				fclose(nonpdrm);
			}
			else {
				fprintf(stderr, "err: failed to create nonpdrm license.\n");
				return -1;
			}
		}
		else {
			fprintf(stderr, "err: failed to decrypt klicensee (check paramaters)\n");
			return -1;
		}

	}
	else {
		fprintf(stderr, "err: no such file: %s\n", OPT_NPDRM_FILE);
		return -1;
	}

	return 0;

}