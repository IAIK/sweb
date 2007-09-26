/**
 * @file debug.h
 */

#ifndef DEBUG_H___
#define DEBUG_H___

#include "types.h"

#define OUTPUT_ENABLED 0x1000000

//group minix
const uint32 MINIX             = 0x00010000 | OUTPUT_ENABLED;
const uint32 M_STORAGE_MANAGER = 0x00010001;
const uint32 M_INODE           = 0x00010002;

//group Block Device
const uint32 BD                 = 0x00020000;
const uint32 BD_MANAGER         = 0x00020001;

//group Console
const uint32 CONSOLE            = 0x00030000;
const uint32 KPRINTF            = 0x00030001;

//group kernel
const uint32 KERNEL             = 0x00040000;
const uint32 CONDITION          = 0x00040001;
const uint32 LOADER             = 0x00040002;
const uint32 SCHEDULER          = 0x00040003;
const uint32 SYSCALL            = 0x00040004;
const uint32 MAIN               = 0x00040005;
const uint32 THREAD             = 0x00040006;

//group mm
const uint32 MM                 = 0x00050000;
const uint32 PM                 = 0x00050001;
const uint32 KMM                 = 0x00050002;



#endif //DEBUG_H___

