#include "SegmentUtils.h"


GDT32Ptr::GDT32Ptr(uint16 gdt_limit, uint32 gdt_addr) :
        limit(gdt_limit),
        addr(gdt_addr)
{}

GDT32Ptr::GDT32Ptr(GDT& gdt) :
        limit(sizeof(gdt.entries) - 1),
        addr((uint32)(size_t)&gdt.entries)
{}

void GDT32Ptr::load()
{
  asm volatile("lgdt %[gdt_ptr]" :: [gdt_ptr]"m"(*this));
}

GDT64Ptr::GDT64Ptr(uint16 gdt_limit, uint64 gdt_addr) :
        limit(gdt_limit),
        addr(gdt_addr)
{}

GDT64Ptr::GDT64Ptr(GDT& gdt) :
        limit(sizeof(gdt.entries) - 1),
        addr((uint64)(size_t)&gdt.entries)
{}

void GDT64Ptr::load()
{
  asm volatile("lgdt %[gdt_ptr]" :: [gdt_ptr]"m"(*this));
}
