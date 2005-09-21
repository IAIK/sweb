/**
 * $Id: types.h,v 1.4 2005/09/21 17:01:12 nomenquis Exp $
 *
 * $Log: types.h,v $
 * Revision 1.3  2005/08/11 16:53:42  nightcreature
 * some addition
 *
 * Revision 1.2  2005/07/31 17:45:06  nightcreature
 * needed updates
 *
 * Revision 1.1  2005/07/10 19:40:00  nightcreature
 * added types used by mini-os code, needs to be changed to our types
 * 
 * Revision 1.0  2005/04/26 19:40:00  rotho
 * copied from: arch/x86/include/
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#define isXenBuild

//start mini-os types...TODO: change source to our types!

typedef signed char         s8;
typedef unsigned char       u8;
typedef signed short        s16;
typedef unsigned short      u16;
typedef signed int          s32;
typedef unsigned int        u32;
typedef signed long long    s64;
typedef unsigned long long  u64;

//typedef unsigned int        size_t; already the same, good

/* FreeBSD compat types */
typedef unsigned char       u_char;
typedef unsigned int        u_int;
typedef unsigned long       u_long;
typedef long long           quad_t;
typedef unsigned long long  u_quad_t;
typedef unsigned int        uintptr_t;

//end mini-os types

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

#define Min(x,y) ((((x))<((y)))?((x)):((y)))
#define Max(x,y) ((((x))>((y)))?((x)):((y)))

//#pragma GCC poison int
//#pragma GCC poison short
//#pragma GCC poison long
//#pragma GCC poison unsigned


//bad hack for proper compiling
//FIXXXME check if this defines are necessary
#define KERNEL_CS (8*3)
#define KERNEL_DS (8*2)
#define KERNEL_SS (8*2)

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
