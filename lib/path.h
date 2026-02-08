#ifndef PATH_H
#define PATH_H 1
#include <stdio.h>

void change_extension(const char* extension, const char* path, size_t path_sz, char* dest);
uint64_t get_filesize(FILE* file);
int file_exists(const char* path);
void read_file(const char* filename, void* data, size_t size);
void write_file(const char* filename, const void* data, size_t size);
void extract_dirname(const char* filename, char* dirname);
void create_directories(const char* dir_name, uint8_t including_last);
void remove_trailing_slash(char* path);

#endif