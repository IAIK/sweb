#include "SegmentUtils.h"


GDT32Ptr::GDT32Ptr(uint16 limit, uint32 addr) :
        limit(limit),
        addr(addr)
{}

GDT32Ptr::GDT32Ptr(GDT& gdt) :
        limit(sizeof(gdt.entries) - 1),
        addr((uint32)(size_t)&gdt.entries)
{}

void GDT32Ptr::load()
{
  asm volatile("lgdt %[gdt_ptr]" :: [gdt_ptr]"m"(*this));
}

GDT64Ptr::GDT64Ptr(uint16 limit, uint64 addr) :
        limit(limit),
        addr(addr)
{}

GDT64Ptr::GDT64Ptr(GDT& gdt) :
        limit(sizeof(gdt.entries) - 1),
        addr((uint64)(size_t)&gdt.entries)
{}

void GDT64Ptr::load()
{
  asm volatile("lgdt %[gdt_ptr]" :: [gdt_ptr]"m"(*this));
}
