/**
 * Filename: FsDefinitions.h
 * Description: this file contains some file-system definitions
 * that are important to all File-System components
 *
 * Created on: 10.06.2012
 * Author: chris
 */

#ifndef FSDEFINITIONS_H_
#define FSDEFINITIONS_H_

#include "types.h"

/**
 * The basic flags for files
 */
#ifndef O_RDONLY
#define O_RDONLY    0x0001
#endif
#ifndef O_WRONLY
#define O_WRONLY    0x0002
#endif
#ifndef O_RDWR
#define O_RDWR      (O_RDONLY|O_WRONLY)
#endif
#ifndef O_CREAT
#define O_CREAT     0x0004
#endif
#ifndef O_APPEND
#define O_APPEND    0x0008
#endif
#ifndef O_EXCL
#define O_EXCL      0x0010
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK  0x0020
#endif
#ifndef O_TRUNC
#define O_TRUNC     0x0040
#endif

#ifndef O_SYNC
#define O_SYNC      0x0080
#endif
#ifndef O_DSYNC
#define O_DSYNC     0x0100
#endif
#ifndef O_RSYNC
#define O_RSYNC     O_SYNC
#endif

/**
 * The basic access modes for files
 */
#define A_READABLE  0x0001
#define A_WRITABLE  0x0002
#define A_EXECABLE  0x0004

/**
 * File-System constants
 */
#define LINK_MAX        0xFFFFFFFF // INT_MAX
#define NAME_MAX        255
#define PATH_MAX        4096
//#define PIPE_BUF        4096

/**
 * File-System Integer Typedefs
 * This typedefs set the used integer size for all used FS-integers
 * NOTE: if you want to implement a File-System with more that 64bit
 * addresses (like ZFS) you have to change the typedef below:
 */

// some type-definitions for the FileSystem
typedef uint32 sector_addr_t;     // sector-addresses (number of sectors)
typedef uint32 sector_len_t;      // length of a sector in bytes
typedef uint32 inode_id_t;        // I-Node id's / numbers (num-i-nodes)

typedef uint32 fd_size_t;
typedef fd_size_t fd_t;

typedef uint32 file_size_t;       // typedef for maximum file-size

typedef uint32 bitmap_t;          // type to hold max number of bits in a FsBitmap

// a UnixTimeStamp
typedef int64 unix_time_stamp;

#endif /* FSDEFINITIONS_H_ */
