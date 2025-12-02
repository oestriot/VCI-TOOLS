#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "compat.h"

uint64_t get_filesize(FILE* file) {
	uint64_t pos = ftell(file);
	fseek(file, 0, SEEK_END);
	uint64_t size = ftell(file);
	fseek(file, pos, SEEK_SET);

	return size;
}

void write_file(const char* filename, const uint8_t* data, size_t size) {
	FILE* fd = fopen(filename, "wb");
	uint64_t wr = fwrite(data, 1, size, fd);
	printf("%s: %llu / %llu (%.0f%%)\n", filename, wr, size, 100.0);
	fclose(fd);
}

void read_file(const char* filename, uint8_t* data, size_t size) {
	FILE* fd = fopen(filename, "rb");
	fread(data, 1, size, fd);
	fclose(fd);
}

int file_exists(const char* path) {
	FILE* fd = fopen(path, "rb");
	if (fd == NULL) {
		return 0;
	}
	else {
		fclose(fd);
		return 1;
	}
}

void change_extension(const char* extension, const char* path, size_t path_sz, char* dest) {
	strncpy(dest, path, path_sz - 1);

	for (size_t i = 0; i < strnlen(dest, path_sz - 1); i++) {
		if (dest[i] == '.') {
			strncpy(dest + i, extension, (path_sz - i) - 1);
			return;
		}
	}
	strncat(dest, extension, path_sz - 1);
}
