#pragma once

#include "types.h"

typedef struct {
    uint16  backlink; //0
    uint16  pad0;
    uint32 esp0;      //1
    uint16  ss0;      //2
    uint16  pad1;
    uint32 esp1;
    uint16  ss1;
    uint16  pad2;
    uint32 esp2;
    uint16  ss2;
    uint16  pad3;
    uint32 cr3;
    uint32 eip;
    uint32 eflags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint16  es;
    uint16  pad4;
    uint16  cs;
    uint16  pad5;
    uint16  ss;
    uint16  pad6;
    uint16  ds;
    uint16  pad7;
    uint16  fs;
    uint16  pad8;
    uint16  gs;
    uint16  pad9;
    uint16  ldt;
    uint16  padA;
    uint16  debugtrap;
    uint16  iobase;
} __attribute__((__packed__))TSS;

typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 typeL           :4;
    uint8 descriptor_type :1;
    uint8 dpl             :2;
    uint8 present         :1;
    uint8 limitH          :4;
    uint8 ignored         :1;
    uint8 typeH           :2;
    uint8 granularity     :1;
    uint8 baseH;
} __attribute__((__packed__))SegmentDescriptor;

// Segment descriptor index in GDT
#define KERNEL_CS_INDEX  3
#define KERNEL_DS_INDEX  2
#define KERNEL_TSS_INDEX 6
#define USER_CS_INDEX 5
#define USER_DS_INDEX 4

// Protection levels
#define DPL_KERNEL  0
#define DPL_USER    3

// Descriptor types
#define D_T_TSS_AVAIL  0x09
#define D_T_DATA       0x10
#define D_T_CODE       0x18

#define D_WRITEABLE   0x02 // For data segments only
#define D_READABLE    0x02 // For code segments only

#define D_DPL0 0x00
#define D_DPL3 0x60

#define D_PRESENT 0x80

#define D_SIZE      0x4000
#define D_G_PAGE    0x8000

// Segment selector values
#define SELECTOR(index, dpl) (((index) << 3) | (dpl))

#define KERNEL_CS  SELECTOR( KERNEL_CS_INDEX,  DPL_KERNEL)
#define KERNEL_DS  SELECTOR( KERNEL_DS_INDEX,  DPL_KERNEL)
#define KERNEL_SS  SELECTOR( KERNEL_DS_INDEX,  DPL_KERNEL)
#define KERNEL_TSS SELECTOR( KERNEL_TSS_INDEX, DPL_KERNEL)
#define USER_CS    SELECTOR( USER_CS_INDEX,    DPL_USER)
#define USER_DS    SELECTOR( USER_DS_INDEX,    DPL_USER)
#define USER_SS    SELECTOR( USER_DS_INDEX,    DPL_USER)


class SegmentUtils
{
public:

  static void initialise();

};

