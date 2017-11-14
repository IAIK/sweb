#include "SegmentUtils.h"
#include "kstring.h"


SegmentDescriptor gdt[7];
struct GDTPtr
{
  uint16 limit;
  uint32 addr;
} __attribute__((__packed__)) gdt_ptr;

TSS *g_tss;

extern "C" void reload_segments()
{
  gdt_ptr.limit = sizeof(gdt) - 1;
  gdt_ptr.addr = (uint32)gdt;
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

static void setSegmentDescriptor(uint32 index, uint32 base, uint32 limit, uint16 type)
{
  gdt[index].baseL  = (uint16)(base & 0xFFFF);
  gdt[index].baseM  = (uint8)((base >> 16U) & 0xFF);
  gdt[index].baseH  = (uint8)((base >> 24U) & 0xFF);
  gdt[index].limitL = (uint16)(limit & 0xFFFF);
  gdt[index].limitH = (uint8) (((limit >> 16U) & 0xF));

  // Bytes 5 + 6 contain segment type
  type &= 0xF0FF;
  *((uint16*)(((uint8*)(gdt + index)) + 5)) |= type;
}

void SegmentUtils::initialise()
{
  setSegmentDescriptor(KERNEL_DS_INDEX, 0, -1U, D_T_DATA | D_DPL0 | D_PRESENT | D_G_PAGE | D_SIZE | D_WRITEABLE);
  setSegmentDescriptor(KERNEL_CS_INDEX, 0, -1U, D_T_CODE | D_DPL0 | D_PRESENT | D_G_PAGE | D_SIZE | D_READABLE);
  setSegmentDescriptor(USER_DS_INDEX, 0, -1U, D_T_DATA | D_DPL3 | D_PRESENT | D_G_PAGE | D_SIZE | D_WRITEABLE);
  setSegmentDescriptor(USER_CS_INDEX, 0, -1U, D_T_CODE | D_DPL3 | D_PRESENT | D_G_PAGE | D_SIZE | D_READABLE);

  g_tss = (TSS*)new uint8[sizeof(TSS)];
  memset((void*)g_tss, 0, sizeof(TSS));
  g_tss->ss0 = KERNEL_SS;
  setSegmentDescriptor(KERNEL_TSS_INDEX, (uint32)g_tss, sizeof(TSS)-1, D_T_TSS_AVAIL | D_DPL0 | D_PRESENT);

  reload_segments();
  int val = KERNEL_TSS;
  asm volatile("ltr %0\n" : : "m" (val));
}
