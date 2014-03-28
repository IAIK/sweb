/**
 * @file debug.h
 */

#ifndef DEBUG_H___
#define DEBUG_H___

#include "types.h"

#define OUTPUT_ENABLED 0x80000000

//group Cache
const uint32 CACHE              = 0x00010000 | OUTPUT_ENABLED;
const uint32 READ_CACHE         = 0x00010001 | OUTPUT_ENABLED;
const uint32 WRITE_CACHE        = 0x00010002 | OUTPUT_ENABLED;

//group Block Device
const uint32 BD                 = 0x00020000 | OUTPUT_ENABLED;
const uint32 BD_MANAGER         = 0x00020001 | OUTPUT_ENABLED;
const uint32 BD_VIRT_DEVICE     = 0x00020002 | OUTPUT_ENABLED;

//group Console
const uint32 CONSOLE            = 0x00040000 | OUTPUT_ENABLED;
const uint32 KPRINTF            = 0x00040001 | OUTPUT_ENABLED;

//group kernel
const uint32 KERNEL             = 0x00080000 | OUTPUT_ENABLED;
const uint32 CONDITION          = 0x00080001 | OUTPUT_ENABLED;
const uint32 LOADER             = 0x00080002 | OUTPUT_ENABLED;
const uint32 SCHEDULER          = 0x00080004 | OUTPUT_ENABLED;
const uint32 SYSCALL            = 0x00080008 | OUTPUT_ENABLED;
const uint32 MAIN               = 0x00080010 | OUTPUT_ENABLED;
const uint32 THREAD             = 0x00080020 | OUTPUT_ENABLED;
const uint32 USERPROCESS        = 0x00080040 | OUTPUT_ENABLED;
const uint32 MOUNTMINIX         = 0x00080080 | OUTPUT_ENABLED;
const uint32 BACKTRACE          = 0x00080100 | OUTPUT_ENABLED;

//group memory management
const uint32 MM                 = 0x00100000 | OUTPUT_ENABLED;
const uint32 PM                 = 0x00100001 | OUTPUT_ENABLED;
const uint32 KMM                = 0x00100002 | OUTPUT_ENABLED;

//group driver
const uint32 DRIVER             = 0x00200000 | OUTPUT_ENABLED;
const uint32 ATA_DRIVER         = 0x00200001 | OUTPUT_ENABLED;
const uint32 IDE_DRIVER         = 0x00200002 | OUTPUT_ENABLED;

//group arch
const uint32 ARCH               = 0x00400000 | OUTPUT_ENABLED;
const uint32 A_COMMON           = 0x00400001 | OUTPUT_ENABLED;
const uint32 A_MEMORY           = 0x00400002 | OUTPUT_ENABLED;
const uint32 A_SERIALPORT       = 0x00400004 | OUTPUT_ENABLED;
const uint32 A_KB_MANAGER       = 0x00400008 | OUTPUT_ENABLED;
const uint32 A_INTERRUPTS       = 0x00400010 | OUTPUT_ENABLED;

//group file system (very general)
const uint32 VFSSYSCALL         = 0x00800000 | OUTPUT_ENABLED;
const uint32 FILE_SYSTEM        = 0x00800001 | OUTPUT_ENABLED;
const uint32 VOLUME_MANAGER     = 0x00800002 | OUTPUT_ENABLED;
const uint32 FS_DEVICE          = 0x00800004 | OUTPUT_ENABLED;
const uint32 FS_BITMAP          = 0x00800008 | OUTPUT_ENABLED;
const uint32 FS_INODE           = 0x00800010 | OUTPUT_ENABLED;
const uint32 FS_UTIL            = 0x00800020 | OUTPUT_ENABLED;

// group: Unix style FileSystems
const uint32 FS_UNIX            = 0x00804000 | OUTPUT_ENABLED;
const uint32 INODE_TABLE        = 0x00804010 | OUTPUT_ENABLED;

// group: concrete FS: Minix
const uint32 FS_MINIX           = 0x00804100 | OUTPUT_ENABLED;

#endif //DEBUG_H___

