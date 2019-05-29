#pragma once

#include "types.h"

#define MSR_FS_BASE        0xC0000100
#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102


void getMSR(uint32 msr, uint32 *lo, uint32 *hi);
void setMSR(uint32 msr, uint32 lo, uint32 hi);
