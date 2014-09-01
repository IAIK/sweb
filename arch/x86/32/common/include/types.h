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

typedef unsigned long long uint64;
typedef long long int64;

typedef uint32 pointer;

typedef uint32 l_off_t;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
typedef uint32 mode_t;
typedef uint32 size_t;
typedef int32 ssize_t;
//#else
//typedef uint32 loff_t;
#endif
typedef uint32 uid_t;
typedef uint32 gid_t;

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#ifndef NO_POISON
//#pragma GCC poison int
//#pragma GCC poison short
//#pragma GCC poison long
//#pragma GCC poison unsigned
#endif

#define KERNEL_CS (8*3)
#define KERNEL_DS (8*2)
#define KERNEL_SS (8*2)
#define DPL_KERNEL  0
#define DPL_USER    3
#define USER_CS ((8*5)|DPL_USER)
#define USER_DS ((8*4)|DPL_USER)
#define USER_SS ((8*4)|DPL_USER)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif
