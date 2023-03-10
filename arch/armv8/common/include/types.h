#pragma once

#include "stdint.h"

typedef int8_t int8;
typedef uint8_t uint8;

typedef int16_t int16;
typedef uint16_t uint16;

typedef int32_t int32;
typedef uint32_t uint32;

typedef uint64_t uint64;
typedef int64_t int64;

typedef uint32 l_off_t;

typedef uint32 mode_t;
typedef uint32 uid_t;
typedef uint32 gid_t;
typedef __SIZE_TYPE__ size_t;

typedef uint64 ppn_t;
typedef size_t vpn_t;

typedef size_t pointer;

/* #pragma GCC poison double float */

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()    __builtin_unreachable()

#define NO_OPTIMIZE __attribute__((optimize("O0")))
