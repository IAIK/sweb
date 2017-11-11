#pragma once

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

typedef uint64 mode_t;
typedef uint64 uid_t;
typedef uint64 gid_t;
typedef uint64 size_t;
typedef int64 ssize_t;

#pragma GCC poison double float

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()    __builtin_unreachable()
