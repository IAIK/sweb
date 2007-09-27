/**
 * @file types.h
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

#ifndef NO_POISON
#pragma GCC poison int
#pragma GCC poison short
#pragma GCC poison long
#pragma GCC poison unsigned
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


/**
 * UNUSED_ARG
 *    If compiler complains about unused argument this
 *    macro may prevent him from complaining.
 *
 */
#define UNUSED_ARG(arg) do{/*nothing*/}while((&arg) == 0)


#endif
