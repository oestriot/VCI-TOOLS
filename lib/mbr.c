#include "mbr.h"
#include <stdio.h>
#include <stdint.h>

const char* format_id_to_name(int format_id) {
	switch (format_id) {
	case ScePartitionType_EXFAT:
		return "exfat";
	case ScePartitionType_FAT16:
		return "fat16";
	case ScePartitionType_RAW:
		return "raw";
	default:
		return "unknown";
	}
}

const char* partition_code_to_name(int partition_code) {
	switch (partition_code) {
	case ScePartitionCode_EMPTY:
		return "empty0";
	case ScePartitionCode_IDSTORAGE:
		return "idstorage0";
	case ScePartitionCode_SLB2:
		return "slb2";
	case ScePartitionCode_OS0:
		return "os0";
	case ScePartitionCode_VS0:
		return "vs0";
	case ScePartitionCode_VD0:
		return "vd0";
	case ScePartitionCode_TM0:
		return "tm0";
	case ScePartitionCode_UR0:
		return "ur0";
	case ScePartitionCode_UX0:
		return "ux0";
	case ScePartitionCode_GRO0:
		return "gro0";
	case ScePartitionCode_GRW0:
		return "grw0";
	case ScePartitionCode_UD0:
		return "ud0";
	case ScePartitionCode_SA0:
		return "sa0";
	case ScePartitionCode_MEDIAID:
		return "mediaid0";
	case ScePartitionCode_PD0:
		return "pd0";
	case ScePartitionCode_UNUSED:
		return "unused0";
	default:
		return "unknown0";
	}
}