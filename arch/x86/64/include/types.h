/**
 * @file types.h
 *
 */

#ifndef TYPES_H_
#define TYPES_H_

typedef char int8;
typedef unsigned char uint8;

typedef short int16;
typedef unsigned short uint16;

typedef int int32;
typedef unsigned int uint32;

typedef long unsigned int uint64;
typedef long int int64;

typedef uint64 pointer;

typedef uint64 l_off_t;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
typedef uint64 mode_t;
typedef uint64 uid_t;
typedef uint64 gid_t;
typedef uint64 size_t;
typedef int64 ssize_t;
#endif

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#ifndef NO_POISON
//#pragma GCC poison int
//#pragma GCC poison short
//#pragma GCC poison long
//#pragma GCC poison unsigned
#endif

#define KERNEL_CS 0x10
#define KERNEL_DS 0x20
#define KERNEL_SS 0x20
#define DPL_KERNEL  0
#define DPL_USER    3
#define USER_CS (0x30|DPL_USER)
#define USER_DS ((0x40)|DPL_USER)
#define USER_SS ((0x40)|DPL_USER)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif
