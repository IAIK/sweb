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

typedef uint32 mode_t;
typedef uint32 uid_t;
typedef uint32 gid_t;
typedef uint32 size_t;
typedef int32 ssize_t;

#define __naked__ __attribute__((naked))

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()    __builtin_unreachable()

#endif
