/**
 * $Id: types.h,v 1.8 2005/05/31 17:29:16 nomenquis Exp $
 *
 * $Log: types.h,v $
 * Revision 1.7  2005/04/24 16:58:03  nomenquis
 * ultra hack threading
 *
 * Revision 1.6  2005/04/23 18:13:26  nomenquis
 * added optimised memcpy and bzero
 * These still could be made way faster by using asm and using cache bypassing mov instructions
 *
 * Revision 1.5  2005/04/23 12:43:09  nomenquis
 * working page manager
 *
 * Revision 1.4  2005/04/22 19:43:04  nomenquis
 *  more poison added
 *
 * Revision 1.3  2005/04/20 21:35:32  nomenquis
 * started to implement page manager
 *
 * Revision 1.2  2005/04/20 15:26:35  nomenquis
 * more and more stuff actually works
 *
 * Revision 1.1  2005/04/12 17:46:43  nomenquis
 * added lots of files
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
#define DPL_KERNEL  0
#define DPL_USER    3
#define USER_CS ((8*5)|DPL_USER)
#define USER_DS ((8*4)|DPL_USER)
#define USER_SS ((8*4)|DPL_USER)


#endif
