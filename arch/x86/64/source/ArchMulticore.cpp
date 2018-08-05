#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"

extern SystemState system_state;

void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
  debug(A_MULTICORE, "Set MSR %x, value: %zx\n", msr, ((size_t)hi << 32) | (size_t)lo);
  asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

uint64 getGSBase()
{
  uint64 gs_base;
  cpuGetMSR(MSR_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
  return gs_base;
}

void setGSBase(uint64 gs_base)
{
  cpuSetMSR(MSR_GS_BASE, gs_base, gs_base >> 32);
}

void setSWAPGSKernelBase(uint64 swapgs_base)
{
  cpuSetMSR(MSR_KERNEL_GS_BASE, swapgs_base, swapgs_base >> 32);
}

void ArchMulticore::setCLS(CoreLocalStorage* cls)
{
  debug(A_MULTICORE, "Set CLS to %p\n", cls);
  cls->cls_ptr = cls;
  cls->core_id = (LocalAPIC::initialized ? local_APIC.getID() : 0);
  setGSBase((uint64)cls);
  setSWAPGSKernelBase((uint64)cls);
}

CoreLocalStorage* ArchMulticore::getCLS()
{
  CoreLocalStorage* cls_ptr;
  assert(getGSBase() != 0); // debug only
  __asm__ __volatile__("movq %%gs:0, %%rax\n"
                       "movq %%rax, %[cls_ptr]\n"
                       : [cls_ptr]"=m"(cls_ptr));
  return cls_ptr;
}

CoreLocalStorage* ArchMulticore::initCLS()
{
  CoreLocalStorage* cls = new CoreLocalStorage{};
  setCLS(cls);
  return getCLS();
}


size_t ArchMulticore::getCoreID()
{
  return getCLS()->core_id;
}

void ArchMulticore::initialize()
{
  CoreLocalStorage* cls = ArchMulticore::initCLS();
  debug(A_MULTICORE, "CLS for core %zu at %p\n", ArchMulticore::getCoreID(), cls);

  startOtherCPUs();
}

extern char apstartup_text_begin;
extern char apstartup_text_end;
extern "C" void apstartup();

struct GDT32Ptr
{
  uint16 limit;
  uint32 addr;
}__attribute__((__packed__));
struct GDT64Ptr
{
  uint16 limit;
  uint64 addr;
}__attribute__((__packed__));
extern GDT32Ptr ap_gdt32_ptr;

extern SegmentDescriptor gdt[7];
extern SegmentDescriptor ap_gdt32[7];

extern uint32 ap_kernel_cr3;
extern char ap_pml4[PAGE_SIZE];

void ArchMulticore::startOtherCPUs()
{
  if(LocalAPIC::initialized)
  {
    debug(A_MULTICORE, "Starting other CPUs\n");

    size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
    debug(A_MULTICORE, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n", &apstartup_text_begin, &apstartup_text_end, apstartup_size);

    pointer apstartup_phys = (pointer)VIRTUAL_TO_PHYSICAL_BOOT(AP_STARTUP_PADDR);
    debug(A_MULTICORE, "apstartup %p, phys: %zx\n", &apstartup, apstartup_phys);
    pointer paddr0 = ArchMemory::getIdentAddress(AP_STARTUP_PADDR);

    auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), paddr0/PAGE_SIZE);
    debug(A_MULTICORE, "paddr0 ppn: %zx\n", m.page_ppn);
    assert(m.page); // TODO: Map if not present
    assert(m.page_ppn == AP_STARTUP_PADDR/PAGE_SIZE);

    debug(A_MULTICORE, "AP GDT32: %p, size: %zx\n", &ap_gdt32, sizeof(ap_gdt32));
    memcpy(&ap_gdt32, &gdt, sizeof(ap_gdt32));


    debug(A_MULTICORE, "AP GDT32 PTR: %p\n", &ap_gdt32_ptr);
    ap_gdt32_ptr.addr = AP_STARTUP_PADDR + ((size_t)&ap_gdt32 - (size_t)&apstartup_text_begin);
    ap_gdt32_ptr.limit = sizeof(ap_gdt32) - 1;
    debug(A_MULTICORE, "AP GDT32 PTR addr: %x\n", ap_gdt32_ptr.addr);
    debug(A_MULTICORE, "AP GDT32 PTR limit: %x\n", ap_gdt32_ptr.limit);

    debug(A_MULTICORE, "AP kernel PML4: %zx\n", (size_t)&ap_pml4);
    memcpy(&ap_pml4, &kernel_page_map_level_4, sizeof(ap_pml4));

    debug(A_MULTICORE, "AP kernel &CR3: %p\n", &ap_kernel_cr3);
    ap_kernel_cr3 = (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);
    debug(A_MULTICORE, "AP kernel CR3 set to: %x (kernel pml4 = %p)\n", ap_kernel_cr3, kernel_page_map_level_4);

    debug(A_MULTICORE, "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n", (void*)&apstartup_text_begin, (void*)&apstartup_text_end, (void*)paddr0, (size_t)AP_STARTUP_PADDR, (size_t)(&apstartup_text_end - &apstartup_text_begin));
    memcpy((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size);
    assert(memcmp((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size) == 0);

    local_APIC.startAPs();
  }
  else
  {
    debug(A_MULTICORE, "No local APIC. Cannot start other CPUs\n");
  }
}








// TODO: Each AP needs its own stack
uint8 ap_stack[0x4000];

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

  uint32 baseH;

  uint32 reserved;
}__attribute__((__packed__)) TSSSegmentDescriptor;

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

extern "C" void __apstartup64()
{
  // Hack to avoid automatic function prologue (stack isn't set up yet)
  // TODO: Use __attribute__((naked)) in GCC 8
  __asm__ __volatile__(".global apstartup64\n"
                       "apstartup64:\n");

  // Load long mode data segments
  __asm__ __volatile__(
    "movw %[K_DS], %%ax\n"
    "movw %%ax, %%ds\n"
    "movw %%ax, %%ss\n"
    "movw %%ax, %%es\n"
    "movw %%ax, %%fs\n"
    "movw %%ax, %%gs\n"
    :
    :[K_DS]"i"(KERNEL_DS)
  );

  __asm__ __volatile__("movq %[stack], %%rsp\n"
                       "movq %[stack], %%rbp\n"
                       :
                       :[stack]"i"(ap_stack + sizeof(ap_stack)));

  debug(A_MULTICORE, "AP startup 64\n");
  debug(A_MULTICORE, "AP switched to stack %p\n", ap_stack + sizeof(ap_stack));

  // Stack variables are messed up in this function because we skipped the function prologue. Should be fine once we've entered another function.
  ArchMulticore::initCore();
}

void ArchMulticore::initCore()
{
  debug(A_MULTICORE, "initAPCore %u\n", local_APIC.getID());

  debug(A_MULTICORE, "AP switching from temp kernel pml4 to main kernel pml4: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4));
  __asm__ __volatile__("movq %[kernel_cr3], %%cr3\n"
                       :
                       :[kernel_cr3]"a"(VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4)));

  debug(A_MULTICORE, "Init core local storage\n");
  CoreLocalStorage* ap_cls = ArchMulticore::initCLS();
  debug(A_MULTICORE, "getCLS(): %p, core id: %zx\n", ArchMulticore::getCLS(), ArchMulticore::getCoreID());

  debug(A_MULTICORE, "AP switching to own GDT at: %p\n", &ap_cls->gdt);
  memcpy(&ap_cls->gdt, ap_gdt32, sizeof(ap_cls->gdt));

  struct GDT64Ptr ap_gdt_ptr;
  ap_gdt_ptr.limit = sizeof(ap_cls->gdt) - 1;
  ap_gdt_ptr.addr = (uint64)&ap_cls->gdt;
  __asm__ __volatile__("lgdt %[gdt]\n"
                       "mov %%ax, %%ds\n"
                       "mov %%ax, %%es\n"
                       "mov %%ax, %%ss\n"
                       "mov %%ax, %%fs\n"
                       :
                       :[gdt]"m"(ap_gdt_ptr), "a"(KERNEL_DS));

  debug(A_MULTICORE, "AP init own TSS at %p\n", &ap_cls->tss);
  setTSSSegmentDescriptor((TSSSegmentDescriptor*)((char*)&ap_cls->gdt + KERNEL_TSS), (size_t)&ap_cls->tss >> 32, (size_t)&ap_cls->tss, sizeof(TSS) - 1, 0);

  size_t ap_stack_top = (size_t)&ap_stack + sizeof(ap_stack);
  ap_cls->tss.ist0 = ap_stack_top;
  ap_cls->tss.rsp0 = ap_stack_top;
  __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::lidt(&InterruptUtils::idtr);

  debug(A_MULTICORE, "Init AP APIC\n");
  local_APIC.setSpuriousInterruptNumber(0xFF);
  local_APIC.initTimer();
  local_APIC.enable(true);

  debug(A_MULTICORE, "Enable AP timer\n");
  //ArchInterrupts::enableTimer();

  while(system_state != RUNNING);

  debug(A_MULTICORE, "Enabling interrupts\n");
  kprintf("Core %u initialized, enabling interrupts\n", local_APIC.getID());
  ArchInterrupts::enableInterrupts();

  while(1)
  {
    debug(A_MULTICORE, "AP %u halting\n", local_APIC.getID());
    __asm__ __volatile__("hlt\n");
  }
}
