/**
 * Filename: debug_print.h
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#ifndef DEBUG_PRINT_H_
#define DEBUG_PRINT_H_

#include <types.h>
#include <stdarg.h>
#include <stdint.h>

#define OUTPUT_ENABLED 0x80000000

//group Cache
const uint32 CACHE              = 0x00010000;// | OUTPUT_ENABLED;
const uint32 READ_CACHE         = 0x00010001;// | OUTPUT_ENABLED;
const uint32 WRITE_CACHE        = 0x00010002;// | OUTPUT_ENABLED;

//group file system (very general)
const uint32 VFSSYSCALL         = 0x00800000;// | OUTPUT_ENABLED;
const uint32 FILE_SYSTEM        = 0x00800001;// | OUTPUT_ENABLED;
const uint32 VOLUME_MANAGER     = 0x00800002;// | OUTPUT_ENABLED;
const uint32 FS_DEVICE          = 0x00800004;// | OUTPUT_ENABLED;
const uint32 FS_BITMAP          = 0x00800008;// | OUTPUT_ENABLED;
const uint32 FS_INODE           = 0x00800010;// | OUTPUT_ENABLED;
const uint32 FS_UTIL            = 0x00800020;
const uint32 FS_TESTCASE        = 0x00800040 | OUTPUT_ENABLED;

// group: Unix style FileSystems
const uint32 FS_UNIX            = 0x00804000;// | OUTPUT_ENABLED;
const uint32 INODE_TABLE        = 0x00804010;// | OUTPUT_ENABLED;

// group: concrete FS: Minix
const uint32 FS_MINIX           = 0x00804100;// | OUTPUT_ENABLED;

void debug(uint32_t flag, const char* fmt, ...);

void kprintfd ( const char *fmt, ... );

#endif /* DEBUG_PRINT_H_ */
