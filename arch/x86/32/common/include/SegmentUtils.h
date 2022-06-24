#pragma once

#include "types.h"

#define KERNEL_DS_ENTRY  2
#define KERNEL_SS_ENTRY  2
#define KERNEL_CS_ENTRY  3
#define USER_DS_ENTRY    4
#define USER_SS_ENTRY    4
#define USER_CS_ENTRY    5
#define KERNEL_TSS_ENTRY 6
#define KERNEL_FS_ENTRY  7
#define KERNEL_GS_ENTRY  7

#define GDT_ENTRY_SIZE 8

#define KERNEL_CS  (GDT_ENTRY_SIZE * KERNEL_CS_ENTRY)
#define KERNEL_DS  (GDT_ENTRY_SIZE * KERNEL_DS_ENTRY)
#define KERNEL_SS  (GDT_ENTRY_SIZE * KERNEL_SS_ENTRY)
#define KERNEL_TSS (GDT_ENTRY_SIZE * KERNEL_TSS_ENTRY)
#define KERNEL_FS  (GDT_ENTRY_SIZE * KERNEL_FS_ENTRY)
#define KERNEL_GS  (GDT_ENTRY_SIZE * KERNEL_GS_ENTRY)

#define DPL_KERNEL  0
#define DPL_USER    3

#define USER_CS ((GDT_ENTRY_SIZE * USER_CS_ENTRY) | DPL_USER)
#define USER_DS ((GDT_ENTRY_SIZE * USER_DS_ENTRY) | DPL_USER)
#define USER_SS ((GDT_ENTRY_SIZE * USER_SS_ENTRY) | DPL_USER)

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
  static void loadKernelSegmentDescriptors();
};


void setFSBase(GDT& gdt, size_t fs_base);
void setGSBase(GDT& gdt, size_t fs_base);
size_t getFSBase(GDT &gdt);
size_t getGSBase(GDT &gdt);

extern GDT gdt;
extern TSS g_tss;
