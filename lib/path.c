#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "compat.h"

uint64_t get_filesize(FILE* file) {
	uint64_t pos = ftell(file);
	fseek(file, 0, SEEK_END);
	uint64_t size = ftell(file);
	fseek(file, pos, SEEK_SET);

	return size;
}
void create_directories(const char* dir_name, uint8_t including_last) {
	char last_dir[0x500] = { 0 };
	for (size_t i = 0; i < strlen(dir_name); i++) {
		if (dir_name[i] == '/') {
			strncpy(last_dir, dir_name, i);
			mkdir(last_dir, 777);
		}
	}

	if (including_last) {
		mkdir(dir_name, 777);
	}
}

void write_file(const char* filename, const void* data, size_t size) {
	create_directories(filename, 0);

	FILE* fd = fopen(filename, "wb");
	size_t wr = fwrite(data, 1, size, fd);
	printf("%s: %zu / %zu (%.0f%%)\n", filename, wr, size, 100.0);
	fclose(fd);
}

void read_file(const char* filename, void* data, size_t size) {
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

void extract_dirname(const char* filename, char* dirname) {
	size_t last_slash = 0;
	for (size_t i = 0; i < strlen(filename); i++) {
		if (filename[i] == '/') {
			last_slash = i;
		}
	}

	strncpy(dirname, filename, last_slash);
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

void remove_trailing_slash(char* path ) {
	size_t path_sz = strlen(path);

	if (path[path_sz - 1] == '/')
		path[path_sz - 1] = '\0';
}