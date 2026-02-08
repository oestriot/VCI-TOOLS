#include "mbr.h"
#include "vci.h"
#include "psv.h"
#include "path.h"

#include "ff/diskio.h"
#include "ff/ff.h"

static uint8_t buffer[SECTOR_SIZE * 0x5000];
#define RETERR() {res = 0; break;}

int read_recursive(const char* prev, const char* extract_dir) {
	int res = 1;

	char path[MAX_PATH] = { 0 };
	char out_path[MAX_PATH] = { 0 };


	FIL fl = { 0 };
	DIR dr = { 0 };
	FILINFO fileinfo = { 0 };

	if (f_opendir(&dr, prev) == FR_OK) {
		remove_trailing_slash((char*)prev);
		remove_trailing_slash((char*)extract_dir);

		do {
			if (f_readdir(&dr, &fileinfo) != FR_OK || fileinfo.fname[0] == '\0') break;

			snprintf(path, sizeof(path) - 1, "%s/%s", prev, fileinfo.fname);
			snprintf(out_path, sizeof(out_path) - 1, "%s%s/%s", extract_dir, prev, fileinfo.fname);

			if ((fileinfo.fattrib & AM_DIR) != 0) {
				printf("%s: %llu / %llu (%.0f%%)\n", out_path, 1llu, 1llu, 100.0f);
				create_directories(out_path, 1);

				res = read_recursive(path, extract_dir);
				if(!res) RETERR();

			}
			if ((fileinfo.fattrib & AM_DIR) == 0) {
				create_directories(out_path, 0);
				if (f_open(&fl, path, FA_READ) == FR_OK) {
					FILE* out_fd = fopen(out_path, "wb");

					if (out_fd != NULL) {
						uint32_t rd = 0;
						uint64_t total = 0;

						do {
							f_read(&fl, buffer, sizeof(buffer), &rd);
							total += fwrite(buffer, 1, rd, out_fd);
							printf("%s: %llu / %llu (%.0f%%)\r", out_path, total, fileinfo.fsize, (((double)total / (double)fileinfo.fsize) * 100.0));
						} while (total < fileinfo.fsize);
						printf("\n");

						fclose(out_fd);
						f_close(&fl);
					} else RETERR();
				} else RETERR();

			}
		} while (1);

		f_closedir(&dr);
	} else { 
		return 0;
	}

	return res;

}


