#ifndef EXFAT_H
#define EXFAT_H 1
#include <stdio.h>
#include <stdint.h>

#include "ff/ffconf.h"
#include "ff/ff.h"
#include "ff/diskio.h"

const char* partition_code_to_name(int partition_code);
const char* format_id_to_name(int format_id);
int read_recursive(const char* prev, const char* extract_dir);

#endif