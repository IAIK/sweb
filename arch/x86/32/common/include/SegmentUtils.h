#pragma once

#include "types.h"

#define KERNEL_CS  (8*3)
#define KERNEL_DS  (8*2)
#define KERNEL_SS  (8*2)
#define KERNEL_TSS (8*6)
#define KERNEL_FS  (8*7)
#define KERNEL_GS  (8*7)
#define DPL_KERNEL  0
#define DPL_USER    3
#define USER_CS ((8*5)|DPL_USER)
#define USER_DS ((8*4)|DPL_USER)
#define USER_SS ((8*4)|DPL_USER)

struct SegmentDescriptor
{
        uint16 limitL;
        uint16 baseL;

        uint8 baseM;
        uint8 typeL;
        uint8 limitH :4;
        uint8 typeH :4;
        uint8 baseH;

        size_t getBase();
        void setBase(size_t base);
}__attribute__((__packed__));

struct GDT
{
        SegmentDescriptor entries[8];
} __attribute__((__packed__));

struct GDT32Ptr
{
        uint16 limit;
        uint32 addr;

        GDT32Ptr() = default;
        GDT32Ptr(uint16 limit, uint32 addr);
        explicit GDT32Ptr(GDT& gdt);
        void load();
}__attribute__((__packed__));

typedef struct
{
        uint32 limitL          : 16;
        uint32 baseLL          : 16;

        uint32 baseLM          :  8;
        uint32 type            :  4;
        uint32 zero            :  1;
        uint32 dpl             :  2;
        uint32 present         :  1;
        uint32 limitH          :  4;
        uint32 avl_to_software :  1;
        uint32 ignored         :  2;
        uint32 granularity     :  1;
        uint32 baseLH          :  8;

}__attribute__((__packed__)) TSSSegmentDescriptor;

void setTSSSegmentDescriptor(TSSSegmentDescriptor* descriptor, uint32 base, uint32 limit, uint8 dpl);


struct TSS
{
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

    void setTaskStack(size_t stack_top);
} __attribute__((__packed__));

class SegmentUtils
{
public:

  static void initialise();

};


void setFSBase(size_t fs_base);
void setGSBase(size_t fs_base);
size_t getFSBase();
size_t getGSBase();


extern GDT gdt;
extern TSS g_tss;
