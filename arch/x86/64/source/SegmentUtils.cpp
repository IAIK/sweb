#include "SegmentUtils.h"
#include "MSR.h"
#include "assert.h"
#include "debug.h"
#include "kstring.h"


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
  asm("lgdt %[gdt_ptr]\n"
      :
      :[gdt_ptr]"m"(*this));
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
  asm("lgdt %[gdt_ptr]\n"
      :
      :[gdt_ptr]"m"(*this));
}


void setTSSSegmentDescriptor(TSSSegmentDescriptor* descriptor, uint32 baseH, uint32 baseL, uint32 limit, uint8 dpl)
{
        debug(A_MULTICORE, "setTSSSegmentDescriptor at %p, baseH: %x, baseL: %x, limit: %x, dpl: %x\n", descriptor, baseH, baseL, limit, dpl);
        memset(descriptor, 0, sizeof(TSSSegmentDescriptor));
        descriptor->baseLL = (uint16) (baseL & 0xFFFF);
        descriptor->baseLM = (uint8) ((baseL >> 16U) & 0xFF);
        descriptor->baseLH = (uint8) ((baseL >> 24U) & 0xFF);
        descriptor->baseH = baseH;
        descriptor->limitL = (uint16) (limit & 0xFFFF);
        descriptor->limitH = (uint8) (((limit >> 16U) & 0xF));
        descriptor->type = 0b1001;
        descriptor->dpl = dpl;
        descriptor->granularity = 0;
        descriptor->present = 1;
}

void TSS::setTaskStack(size_t stack_top)
{
        ist0 = stack_top;
        rsp0 = stack_top;
}


size_t getGSBase()
{
        size_t gs_base;
        MSR::getMSR(MSR_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
        return gs_base;
}

size_t getGSKernelBase()
{
        size_t gs_base;
        MSR::getMSR(MSR_KERNEL_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
        return gs_base;
}

size_t getFSBase()
{
        size_t fs_base;
        MSR::getMSR(MSR_FS_BASE, (uint32*)&fs_base, ((uint32*)&fs_base) + 1);
        return fs_base;
}


void setGSBase(size_t gs_base)
{
    MSR::setMSR(MSR_GS_BASE, gs_base, gs_base >> 32);
}

void setFSBase(size_t fs_base)
{
    MSR::setMSR(MSR_FS_BASE, fs_base, fs_base >> 32);
}

void setSWAPGSKernelBase(size_t swapgs_base)
{
    MSR::setMSR(MSR_KERNEL_GS_BASE, swapgs_base, swapgs_base >> 32);
}

void* getSavedFSBase()
{
        void* fs_base;
        __asm__ __volatile__("movq %%gs:0, %%rax\n"
                             "movq %%rax, %[fs_base]\n"
                             : [fs_base]"=m"(fs_base)
                             :
                             : "rax");
        assert(fs_base != 0);
        return fs_base;
}

void restoreSavedFSBase()
{
    setFSBase((uint64)getSavedFSBase());
}
