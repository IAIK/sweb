/**
 * $Id: types.h,v 1.1 2005/04/26 19:40:00 rotho Exp $
 *
 * $Log: types.h,v $
 *
 *
 */

#ifndef _TYPES_H_
#define _TYPES_H_

typedef char int8;
typedef unsigned char uint8;

typedef short int16;
typedef unsigned short uint16;

typedef int int32;
typedef unsigned int uint32;

typedef unsigned long long uint64;
typedef long long int64;

typedef unsigned int pointer;

typedef unsigned int size_t;

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#pragma GCC poison int
#pragma GCC poison short
#pragma GCC poison long
#pragma GCC poison unsigned

#define KERNEL_CS (8*3)
#define KERNEL_DS (8*2)
#define KERNEL_SS (8*2)


#endif
