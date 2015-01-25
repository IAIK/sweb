/**
 * @file types.h
 *
 */

#ifndef _MINIXFSTYPES_H_
#define _MINIXFSTYPES_H_

#include <cstdio>
#include <sys/types.h>

#define kprintf(fmt,args...) do { printf(fmt, ## args); } while (0)
#define debug(flag,fmt,args...) do { if (flag & 0x80000000) { printf(fmt,## args); } } while(0)

#include "../../common/include/console/debug.h"

typedef int8_t int8;
typedef u_int8_t uint8;

typedef int16_t int16;
typedef u_int16_t uint16;

typedef int32_t int32;
typedef u_int32_t uint32;

typedef u_int64_t uint64;
typedef int64_t int64;

//typedef u_int32_t size_t;

//#define Min(x,y) (((x)<(y))?(x):(y))
//#define Max(x,y) (((x)>(y))?(x):(y))

#endif
