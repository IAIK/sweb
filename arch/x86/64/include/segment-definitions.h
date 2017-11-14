#pragma once

#include "types.h"


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
// Hardware multiplies index by 8 to get GDT offset, but we want 16 bytes for each index -> multiply with 2
#define SELECTOR(index, dpl) ((((index)*2) << 3) | (dpl))

#define KERNEL_CS  SELECTOR( KERNEL_CS_INDEX,  DPL_KERNEL )
#define KERNEL_DS  SELECTOR( KERNEL_DS_INDEX,  DPL_KERNEL )
#define KERNEL_SS  SELECTOR( KERNEL_DS_INDEX,  DPL_KERNEL )
#define KERNEL_TSS SELECTOR( KERNEL_TSS_INDEX, DPL_KERNEL )
#define USER_CS    SELECTOR( USER_CS_INDEX,    DPL_USER )
#define USER_DS    SELECTOR( USER_DS_INDEX,    DPL_USER )
#define USER_SS    SELECTOR( USER_DS_INDEX,    DPL_USER )


// System + gate descriptor types
#define D_T_TSS_AVAIL  0x09
#define D_T_DATA       0x10
#define D_T_CODE       0x18

#define D_WRITEABLE   0x02 // For data segments only
#define D_READABLE    0x02 // For code segments only

// General segment descriptor type flags
#define D_DPL0 0x00
#define D_DPL3 0x60

#define D_PRESENT 0x80

#define D_64BIT     0x2000
#define D_SIZE      0x4000
#define D_G_PAGE    0x8000


typedef struct
{
    uint16 limitL;
    uint16 baseLL;
    uint8 baseLM;
    uint8 typeL           :4;
    uint8 descriptor_type :1;
    uint8 dpl             :2;
    uint8 present         :1;
    uint8 limitH          :4;
    uint8 ignored         :1;
    uint8 typeH           :2;
    uint8 granularity     :1;
    uint8 baseLH;
    uint32 baseH;
    uint32 reserved;
}__attribute__((__packed__)) SegmentDescriptor;
