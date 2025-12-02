#ifndef COMPAT_H
#define COMPAT_H 1

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN 1
#include <stdio.h>
#include <windows.h>

#define ftell _ftelli64
#define main utf8_main
#define fopen utf8_fopen
FILE* utf8_fopen(const char* filename, const char* mode);
int utf8_main(int argc, char** argv);
#endif

#endif

#ifdef __GNUC__
#define PACK( declaration ) declaration __attribute__((__packed__))
#elif _MSC_VER
#define PACK( declaration ) __pragma(pack(push, 1) ) declaration __pragma(pack(pop))
#endif
