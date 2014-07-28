/**
 * @file SegmentUtils.cpp
 *
 */

#include "SegmentUtils.h"
#include "ArchCommon.h"

extern uint32 tss_selector;
extern uint32 gdt_ptr_new;

extern "C" void reload_segements();

typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 type;
    uint8 limitH;
    uint8 baseH;
} __attribute__((__packed__))SegDesc;


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

TSS *g_tss;


#define GDT_ENTRY_NUM 8192
#define GDT_SIZE      (GDT_ENTRY_NUM * 8)

#define SEGMENT_ABSENT  0x00
#define SEGMENT_PRESENT 0x80
#define SEGMENT_DPL0    0x00
#define SEGMENT_DPL1    0x20
#define SEGMENT_DPL2    0x40
#define SEGMENT_DPL3    0x60

static void setTSSSegDesc(uint32 base, uint32 limit, uint8 type) 
{
    SegDesc *desc = (SegDesc*)&tss_selector;

    desc->baseL  = (uint16)(base & 0xFFFF);
    desc->baseM  = (uint8)((base >> 16U) & 0xFF);
    desc->baseH  = (uint8)((base >> 24U) & 0xFF);
    desc->limitL = (uint16)(limit & 0xFFFF);
    // 4KB unit & 32bit segment
    desc->limitH = (uint8) (((limit >> 16U) & 0x0F) | 0xC0); 
    desc->type   = type;
    return;
}

void SegmentUtils::initialise()
{
  g_tss = (TSS*)new uint8[sizeof(TSS)]; // new uint8[sizeof(TSS)];
  ArchCommon::bzero((pointer)g_tss,sizeof(TSS));

  g_tss->ss0 = KERNEL_SS;
  setTSSSegDesc((uint32)g_tss, 0x00000067, SEGMENT_PRESENT | SEGMENT_DPL0 | 0x00 | 0x09);

  // we have to reload our segment stuff
  uint16 val = 8 * 6;

  reload_segements();

  g_tss->ss0  = KERNEL_SS;

  // now use our damned tss
  asm volatile("ltr %0\n": "=m" (val));
}
