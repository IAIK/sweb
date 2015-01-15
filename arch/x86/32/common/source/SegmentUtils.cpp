/**
 * @file SegmentUtils.cpp
 *
 */

#include "SegmentUtils.h"
#include "ArchCommon.h"

extern uint32 tss_selector;
extern uint32 gdt_ptr_new;

typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 type;
    uint8 limitH;
    uint8 baseH;
} __attribute__((__packed__))SegDesc;

TSS *g_tss;


#define GDT_ENTRY_NUM 8192
#define GDT_SIZE      (GDT_ENTRY_NUM * 8)

#define SEGMENT_ABSENT  0x00
#define SEGMENT_PRESENT 0x80
#define SEGMENT_DPL0    0x00
#define SEGMENT_DPL1    0x20
#define SEGMENT_DPL2    0x40
#define SEGMENT_DPL3    0x60

extern uint32 gdt_ptr_very_new;

extern "C" void reload_segments()
{
  // reload the gdt with the newly set up segments
  asm("lgdt (%[gdt_ptr])" : : [gdt_ptr]"m"(gdt_ptr_very_new));
  // now prepare all the segment registers to use our segments
  asm("mov %%ax, %%ds\n"
      "mov %%ax, %%es\n"
      "mov %%ax, %%ss\n"
      "mov %%ax, %%fs\n"
      "mov %%ax, %%gs\n"
      : : "a"(KERNEL_DS));
  // jump onto the new code segment
  asm("ljmp %[cs],$1f\n"
      "1:": : [cs]"i"(KERNEL_CS));
}

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

  reload_segments();

  g_tss->ss0  = KERNEL_SS;

  // now use our damned tss
  asm volatile("ltr %0\n": "=m" (val));
}
