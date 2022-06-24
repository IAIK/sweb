#include "SegmentUtils.h"
#include "kstring.h"
#include "assert.h"
#include "debug.h"
#include "ArchMulticore.h"
#include "Scheduler.h"

GDT gdt;
TSS g_tss;

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
            ::[gdt_ptr]"m"(*this));
}

size_t SegmentDescriptor::getBase()
{
        return ((size_t)baseH << 24) | ((size_t)baseM << 16) | (size_t)baseL;
}

void SegmentDescriptor::setBase(size_t base)
{
    baseL = base & 0xFFFF;
    baseM = (base >> 16) & 0xFF;
    baseH = (base >> 24) & 0xFF;
}

void SegmentDescriptor::setLimit(size_t limit)
{
    limitL = (uint16)(limit & 0xFFFF);
    limitH = (uint8) (((limit >> 16U) & 0xF));
}

static void setSegmentDescriptor(uint32 index, uint32 base, uint32 limit, uint8 dpl, uint8 code, uint8 tss)
{
  gdt.entries[index].setBase(base);
  gdt.entries[index].setLimit(limit);
  gdt.entries[index].typeH  = 0xC; // 4kb + 32bit
  gdt.entries[index].typeL  = (tss ? 0x89 : 0x92) | (dpl << 5) | (code ? 0x8 : 0); // present bit + memory expands upwards + code
}

void setTSSSegmentDescriptor(TSSSegmentDescriptor* descriptor, uint32 base, uint32 limit, uint8 dpl)
{
        debug(A_MULTICORE, "setTSSSegmentDescriptor at %p, base: %x, limit: %x, dpl: %x\n", descriptor, base, limit, dpl);
        memset(descriptor, 0, sizeof(TSSSegmentDescriptor));
        descriptor->baseLL = (uint16) (base & 0xFFFF);
        descriptor->baseLM = (uint8) ((base >> 16U) & 0xFF);
        descriptor->baseLH = (uint8) ((base >> 24U) & 0xFF);
        descriptor->limitL = (uint16) (limit & 0xFFFF);
        descriptor->limitH = (uint8) (((limit >> 16U) & 0xF));
        descriptor->type = 0b1001;
        descriptor->dpl = dpl;
        descriptor->granularity = 0;
        descriptor->present = 1;
}

void TSS::setTaskStack(size_t stack_top)
{
        ss0 = KERNEL_SS;
        esp0 = stack_top;
}


void SegmentUtils::initialise()
{
  setSegmentDescriptor(KERNEL_DS_ENTRY, 0, -1U, 0, 0, 0);
  setSegmentDescriptor(KERNEL_CS_ENTRY, 0, -1U, 0, 1, 0);
  setSegmentDescriptor(USER_DS_ENTRY,   0, -1U, 3, 0, 0);
  setSegmentDescriptor(USER_CS_ENTRY,   0, -1U, 3, 1, 0);
  setSegmentDescriptor(KERNEL_FS_ENTRY, 0, -1U, 0, 0, 0);

  memset(&g_tss, 0, sizeof(TSS));
  g_tss.ss0 = KERNEL_SS;
  setSegmentDescriptor(KERNEL_TSS_ENTRY, (uint32)&g_tss, sizeof(TSS)-1, 0, 0, 1);

  // we have to reload our segment stuff
  GDT32Ptr(gdt).load();

  loadKernelSegmentDescriptors();

  int val = KERNEL_TSS;
  asm volatile("ltr %0\n" : : "m" (val));

  debug(A_MULTICORE, "Setting temporary CLS for boot processor\n");
  extern char cls_start;

  CPULocalStorage::setCLS(gdt, &cls_start);
  currentThread = nullptr;
}

void SegmentUtils::loadKernelSegmentDescriptors()
{
    // gs+fs set to normal data segment for now, set to cpu local storage when cls is initialized
    asm("mov %[ds], %%ds\n"
        "mov %[ds], %%es\n"
        "mov %[ds], %%ss\n"
        "mov %[ds], %%fs\n"
        "mov %[ds], %%gs\n"
        "ljmp %[cs], $1f\n"
        "1:\n"
        ::[ds]"a"(KERNEL_DS),
          [cs]"i"(KERNEL_CS));
}

size_t getGSBase(GDT& gdt)
{
    return gdt.entries[KERNEL_GS_ENTRY].getBase();
}

size_t getFSBase(GDT& gdt)
{
    return gdt.entries[KERNEL_FS_ENTRY].getBase();
}


void setGSBase(GDT& gdt, size_t gs_base)
{
    gdt.entries[KERNEL_FS_ENTRY].setBase(gs_base);
    asm("mov %[gs], %%gs\n"
        :
        :[gs]"a"(KERNEL_GS));
}

void setFSBase(GDT& gdt, size_t fs_base)
{
    gdt.entries[KERNEL_FS_ENTRY].setBase(fs_base);
    asm("mov %[fs], %%fs\n"
        :
        :[fs]"a"(KERNEL_FS));
}
