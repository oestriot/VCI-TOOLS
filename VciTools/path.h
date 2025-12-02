#ifndef PATH_H
#define PATH_H 1
#include <stdio.h>

void change_extension(const char* extension, const char* path, size_t path_sz, char* dest);
uint64_t get_filesize(FILE* file);
int file_exists(const char* path);
void read_file(const char* filename, uint8_t* data, size_t size);
void write_file(const char* filename, const uint8_t* data, size_t size);

#endif