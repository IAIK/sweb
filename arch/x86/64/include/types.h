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

// Segment descriptor index in GDT
#define KERNEL_CS_INDEX 1
#define KERNEL_DS_INDEX 2 // Hardcoded to 2 in arch_interrupts.S
#define KERNEL_TSS_INDEX 5
#define USER_CS_INDEX 3
#define USER_DS_INDEX 4

// Protection levels
#define DPL_KERNEL  0
#define DPL_USER    3

// Segment selector values
#define KERNEL_CS  (KERNEL_CS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_DS  (KERNEL_DS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_SS  (KERNEL_DS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_TSS (KERNEL_TSS_INDEX * sizeof(SegmentDescriptor))
#define USER_CS ((USER_CS_INDEX*sizeof(SegmentDescriptor)) | DPL_USER)
#define USER_DS ((USER_DS_INDEX*sizeof(SegmentDescriptor)) | DPL_USER)
#define USER_SS ((USER_DS_INDEX*sizeof(SegmentDescriptor)) | DPL_USER)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()    __builtin_unreachable()

typedef struct
{
    uint16 limitL;
    uint16 baseLL;
    uint8 baseLM;
    uint8 typeL;
    uint8 limitH :4;
    uint8 typeH :4;
    uint8 baseLH;
    uint32 baseH;
    uint32 reserved;
}__attribute__((__packed__)) SegmentDescriptor;

