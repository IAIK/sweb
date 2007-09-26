#ifndef DEBUG_H___
#define DEBUG_H___

#include "types.h"

#define OUTPUT_ENABLED 0x1000000

const uint32 MINIX             = 0x00010000 | OUTPUT_ENABLED;
const uint32 M_STORAGE_MANAGER = 0x00010001;
const uint32 M_INODE           = 0x00010002;

#endif //DEBUG_H___

