/**
 * @file debug.h
 */

#ifndef DEBUG_H___
#define DEBUG_H___

#include "types.h"

#define OUTPUT_ENABLED 0x80000000

//group Cache
const uint32 CACHE              = 0x00010000;// | OUTPUT_ENABLED;
const uint32 READ_CACHE         = 0x00010001;// | OUTPUT_ENABLED;
const uint32 WRITE_CACHE        = 0x00010002;// | OUTPUT_ENABLED;

//group Block Device
const uint32 BD                 = 0x00020000;
const uint32 BD_MANAGER         = 0x00020001;
const uint32 BD_VIRT_DEVICE     = 0x00020002;

//group Console
const uint32 CONSOLE            = 0x00040000;
const uint32 KPRINTF            = 0x00040001;

//group kernel
const uint32 KERNEL             = 0x00080000;
const uint32 CONDITION          = 0x00080001;
const uint32 LOADER             = 0x00080002;
const uint32 SCHEDULER          = 0x00080004 | OUTPUT_ENABLED;
const uint32 SYSCALL            = 0x00080008 | OUTPUT_ENABLED;
const uint32 MAIN               = 0x00080010 | OUTPUT_ENABLED;
const uint32 THREAD             = 0x00080020 | OUTPUT_ENABLED;
const uint32 USERPROCESS        = 0x00080040 | OUTPUT_ENABLED;
const uint32 MOUNTMINIX         = 0x00080080 | OUTPUT_ENABLED;
const uint32 BACKTRACE          = 0x00080100 | OUTPUT_ENABLED;
const uint32 USERTRACE          = 0x00080200 | OUTPUT_ENABLED;
//group memory management
const uint32 MM                 = 0x00100000;
const uint32 PM                 = 0x00100001 | OUTPUT_ENABLED;
const uint32 KMM                = 0x00100002;

//group driver
const uint32 DRIVER             = 0x00200000;
const uint32 ATA_DRIVER         = 0x00200001;
const uint32 IDE_DRIVER         = 0x00200002;
const uint32 MMC_DRIVER         = 0x00200004;
//group arch
const uint32 ARCH               = 0x00400000;
const uint32 A_COMMON           = 0x00400001;
const uint32 A_MEMORY           = 0x00400002;
const uint32 A_SERIALPORT       = 0x00400004;
const uint32 A_KB_MANAGER       = 0x00400008;
const uint32 A_INTERRUPTS       = 0x00400010;

//group file system (very general)
const uint32 VFSSYSCALL         = 0x00800000;
const uint32 FILE_SYSTEM        = 0x00800001;
const uint32 VOLUME_MANAGER     = 0x00800002;
const uint32 FS_DEVICE          = 0x00800004;
const uint32 FS_BITMAP          = 0x00800008;
const uint32 FS_INODE           = 0x00800010;
const uint32 FS_UTIL            = 0x00800020;

// group: Unix style FileSystems
const uint32 FS_UNIX            = 0x00804000;
const uint32 INODE_TABLE        = 0x00804010;

// group: concrete FS: Minix
const uint32 FS_MINIX           = 0x00804100;

#endif //DEBUG_H___

