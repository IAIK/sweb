#include "SegmentUtils.h"
#include "kstring.h"

typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 typeL;
    uint8 limitH : 4;
    uint8 typeH  : 4;
    uint8 baseH;
} __attribute__((__packed__))SegmentDescriptor;

SegmentDescriptor gdt[7];
struct GDTPtr
{
  uint16 limit;
  uint32 addr;
} __attribute__((__packed__)) gdt_ptr;

TSS *g_tss;

extern "C" void reload_segments()
{
  // reload the gdt with the newly set up segments
  asm("lgdt (%[gdt_ptr])" : : [gdt_ptr]"m"(gdt_ptr));
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

static void setSegmentDescriptor(uint32 index, uint32 base, uint32 limit, uint8 dpl, uint8 code, uint8 tss)
{
    gdt[index].baseL  = (uint16)(base & 0xFFFF);
    gdt[index].baseM  = (uint8)((base >> 16U) & 0xFF);
    gdt[index].baseH  = (uint8)((base >> 24U) & 0xFF);
    gdt[index].limitL = (uint16)(limit & 0xFFFF);
    gdt[index].limitH = (uint8) (((limit >> 16U) & 0xF));
    gdt[index].typeH  = 0xC; // 4kb + 32bit
    gdt[index].typeL  = (tss ? 0x89 : 0x92) | (dpl << 5) | (code ? 0x8 : 0); // present bit + memory expands upwards + code
}

void SegmentUtils::initialise()
{
  setSegmentDescriptor(KERNEL_DS_INDEX, 0, -1U, DPL_KERNEL, 0, 0);
  setSegmentDescriptor(KERNEL_CS_INDEX, 0, -1U, DPL_KERNEL, 1, 0);
  setSegmentDescriptor(USER_DS_INDEX, 0, -1U, DPL_USER, 0, 0);
  setSegmentDescriptor(USER_CS_INDEX, 0, -1U, DPL_USER, 1, 0);

  g_tss = (TSS*)new uint8[sizeof(TSS)];
  memset((void*)g_tss, 0, sizeof(TSS));
  g_tss->ss0 = KERNEL_SS;
  setSegmentDescriptor(KERNEL_TSS_INDEX, (uint32)g_tss, sizeof(TSS)-1, DPL_KERNEL, 0, 1);
  // we have to reload our segment stuff
  gdt_ptr.limit = sizeof(gdt) - 1;
  gdt_ptr.addr = (uint32)gdt;
  reload_segments();
  int val = KERNEL_TSS;
  asm volatile("ltr %0\n" : : "m" (val));
}
