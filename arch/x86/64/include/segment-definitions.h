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
#define KERNEL_CS  (KERNEL_CS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_DS  (KERNEL_DS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_SS  (KERNEL_DS_INDEX  * sizeof(SegmentDescriptor))
#define KERNEL_TSS (KERNEL_TSS_INDEX * sizeof(SegmentDescriptor))
#define USER_CS   ((USER_CS_INDEX    * sizeof(SegmentDescriptor)) | DPL_USER)
#define USER_DS   ((USER_DS_INDEX    * sizeof(SegmentDescriptor)) | DPL_USER)
#define USER_SS   ((USER_DS_INDEX    * sizeof(SegmentDescriptor)) | DPL_USER)


// System + gate descriptor types
#define D_T_LDT        0x02
#define D_T_TSS_AVAIL  0x09
#define D_T_TSS_BUSY   0x0B
#define D_T_CALL_GATE  0x0C
#define D_T_INT_GATE   0x0E
#define D_T_TRAP_GATE  0x0F

// Code/Data descriptor types
#define D_T_DATA      0x10
#define D_T_CODE      0x18

#define D_ACCESSED    0x01
#define D_WRITEABLE   0x02 // For data segments only
#define D_READABLE    0x02 // For code segments only
#define D_EXPAND_DOWN 0x04 // For data segments only
#define D_CONFORMING  0x04 // For code segments only

// General segment descriptor type flags
#define D_DPL0 0x00
#define D_DPL1 0x20
#define D_DPL2 0x40
#define D_DPL3 0x60

#define D_PRESENT 0x80

#define D_64BIT     0x2000
#define D_SIZE      0x4000
#define D_G_PAGE    0x8000



typedef struct
{
    uint16 rpl     : 2;
    uint16 ldt     : 1; // Use local descriptor table
    uint16 offset  :13;
}__attribute__((__packed__)) SegmentSelector;


typedef struct
{
    uint16 limitL;           // Limit is ignored for data segments in long mode
    uint16 baseLL;
    uint8 baseLM;
    uint8 accessed       :1;
    uint8 writeable      :1;
    uint8 direction      :1; // 0 = up, 1 = down
    uint8 is_code_seg    :1; // 0 = data segment, 1 = code segment
    uint8 is_normal_seg  :1; // 0 = data/code segment, 1 = system segment
    uint8 dpl            :2;
    uint8 present        :1;
    uint8 limitH         :4;
    uint8 ignored        :1;
    uint8 zero           :1;
    uint8 stack_seg_size :1; // Size of expand-down stack segments
    uint8 granularity    :1; // 0 = limit is byte sized, 1 = limit is page (4k) sized
    uint8 baseLH;
    uint32 baseH;            // Only actually exists in TSS and LDT descriptors in long mode
    uint32 reserved;         // Only actually exists in TSS and LDT descriptors in long mode
}__attribute__((__packed__)) DataSegmentDescriptor;

typedef struct
{
    uint16 limitL;          // Limit is ignored for code segments in long mode
    uint16 baseLL;
    uint8 baseLM;
    uint8 accessed      :1;
    uint8 readable      :1;
    uint8 conforming    :1;
    uint8 is_code_seg   :1; // 0 = data segment, 1 = code segment
    uint8 is_normal_seg :1; // 0 = data/code segment, 1 = system segment
    uint8 dpl           :2;
    uint8 present       :1;
    uint8 limitH        :4;
    uint8 ignored       :1;
    uint8 long_mode     :1; // 64-bit flag (0 = compatibility mode, 1 = normal 64-bit mode)
    uint8 addr_size     :1; // Default data/address size in compatibility mode. (0 = 16 bit, 1 = 32 bit). Must be 0 when long_mode = 1
    uint8 granularity   :1;
    uint8 baseLH;
    uint32 baseH;           // Only actually exists in TSS and LDT descriptors in long mode
    uint32 reserved;        // Only actually exists in TSS and LDT descriptors in long mode
}__attribute__((__packed__)) CodeSegmentDescriptor;

typedef struct
{
    uint16 limitL;
    uint16 baseLL;
    uint8 baseLM;
    uint8 type          :4; // system segment type (TSS, LDT, ...)
    uint8 is_normal_seg :1; // 0 = system segment, 1 = data/code segment
    uint8 dpl           :2;
    uint8 present       :1;
    uint8 limitH        :4;
    uint8 ignored       :1;
    uint8 zero          :2;
    uint8 granularity   :1;
    uint8 baseLH;
    uint32 baseH;
    uint32 reserved;
}__attribute__((__packed__)) SystemSegmentDescriptor;

typedef union
{
    DataSegmentDescriptor dataseg;
    CodeSegmentDescriptor codeseg;
    SystemSegmentDescriptor sysseg;
}__attribute__((__packed__)) SegmentDescriptor;
