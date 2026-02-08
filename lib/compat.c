#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1

#include "compat.h"
#include <malloc.h>
#include <windows.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <direct.h>

int utf8_mkdir(const char* dirname) {
	wchar_t wdirname[0x1028] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, dirname, strlen(dirname), wdirname, sizeof(wdirname));

	return _wmkdir(wdirname);

}

FILE* utf8_fopen(const char* filename, const char* mode) {

	wchar_t wfname[0x1028] = { 0 };
	wchar_t wmode[0x1028] = { 0 };

	MultiByteToWideChar(CP_UTF8, 0, filename, strlen(filename), wfname, sizeof(wfname));
	MultiByteToWideChar(CP_UTF8, 0, mode, strlen(mode), wmode, sizeof(wfname));

	return _wfopen(wfname, wmode);
}

#ifdef _MSC_VER
int wmain(int argc, wchar_t** argv) {
	setlocale(LC_ALL, ".UTF8");
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	SetFileApisToOEM();

	size_t buffer_size = (sizeof(char*) * (argc + 1));
	char** utf8_argv = alloca(buffer_size);
	if (utf8_argv == NULL) return -1;
	memset(utf8_argv, 0x00, buffer_size);

	for (int i = 0; i < argc; i++) {
		size_t utf8_len = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
		utf8_argv[i] = alloca(utf8_len + 1);
		memset(utf8_argv[i], 0, utf8_len + 1);
		WideCharToMultiByte(CP_UTF8, 0, argv[i], wcslen(argv[i]), utf8_argv[i], utf8_len, NULL, NULL);
	}
	return utf8_main(argc, utf8_argv);
}
#endif

#endif