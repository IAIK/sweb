#pragma once

#include "stdint.h"
#include "klibc/sys/types.h"

typedef int8_t int8;
typedef uint8_t uint8;

typedef int16_t int16;
typedef uint16_t uint16;

typedef int32_t int32;
typedef uint32_t uint32;

typedef uint64_t uint64;
typedef int64_t int64;

typedef uint64_t l_off_t;

typedef uint64_t mode_t;
typedef uint64_t uid_t;
typedef uint64_t gid_t;

typedef __SIZE_TYPE__ size_t;

typedef uint64_t ppn_t;
typedef size_t vpn_t;

typedef size_t pointer;


/* #pragma GCC poison double float */

#define Min(x,y) (((x)<(y))?(x):(y))
#define Max(x,y) (((x)>(y))?(x):(y))

#define KERNEL_CS 0x10
#define KERNEL_DS 0x20
#define KERNEL_SS KERNEL_DS
#define KERNEL_TSS 0x50
#define DPL_KERNEL  0
#define DPL_USER    3
#define USER_CS ((0x30)|DPL_USER)
#define USER_DS ((0x40)|DPL_USER)
#define USER_SS USER_DS

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()    __builtin_unreachable()
