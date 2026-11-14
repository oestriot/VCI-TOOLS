#ifndef COMPAT_H
#define COMPAT_H 1

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN 1
// deal with windows bullshit ..

#include <stdio.h>
#include <stdint.h>
#include <direct.h>
#include <Windows.h>

#define ftell _ftelli64
#define fseek _fseeki64
#define fopen utf8_fopen
#define mkdir(dir, perm) utf8_mkdir(dir)
FILE* utf8_fopen(const char* filename, const char* mode);
int utf8_mkdir(const char* dirname);

#ifdef _MSC_VER
#define main utf8_main
extern int utf8_main(int argc, char** argv);
#endif

#endif

#ifdef _DEBUG
#define LOGGING_ENABLED 1
#endif

#ifdef LOGGING_ENABLED
#define LOG(...) printf(__VA_ARGS__); 
#define LOG_BOOL(label, b) LOG("%s: %s\n", label, b ? "True" : "False");
#define LOG_BUFFER(label, buf, sz) do { LOG("%s:", label); for(size_t i = 0; i < sz; i++) { LOG(" %02X", ((uint8_t*)(buf))[i]); } LOG("\n"); } while(0);
#define LOG_LINE(...) do { LOG(__VA_ARGS__); LOG("\n"); } while(0);
#else 
#define LOG(...) /*...*/
#define LOG_BOOL(...) /*...*/
#define LOG_BUFFER(...) /*...*/
#define LOG_LINE(...) /*...*/
#endif

#define MIN(x, y) (x < y ? x : y)
#ifdef __GNUC__
#define PACK( declaration ) declaration __attribute__((__packed__))
#define swap16 __builtin_bswap16
#define swap32 __builtin_bswap32
#elif _MSC_VER
#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#define PACK( declaration ) __pragma(pack(push, 1) ) declaration __pragma(pack(pop))
#endif

#endif