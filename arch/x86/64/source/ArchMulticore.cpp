#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"
#include "MutexLock.h"

extern SystemState system_state;

ustl::vector<CpuLocalStorage*> ArchMulticore::cpu_list_;
Mutex ArchMulticore::cpu_list_lock_("CPU list lock");
bool ArchMulticore::cpus_started_ = false;

void getMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void setMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
  debug(A_MULTICORE, "Set MSR %x, value: %zx\n", msr, ((size_t)hi << 32) | (size_t)lo);
  asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

uint64 getGSBase()
{
  uint64 gs_base;
  getMSR(MSR_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
  return gs_base;
}

void setGSBase(uint64 gs_base)
{
  setMSR(MSR_GS_BASE, gs_base, gs_base >> 32);
}

void setSWAPGSKernelBase(uint64 swapgs_base)
{
  setMSR(MSR_KERNEL_GS_BASE, swapgs_base, swapgs_base >> 32);
}

CpuLocalStorage::CpuLocalStorage() :
        cls_ptr(init()),
        scheduler(this)
{
  debug(A_MULTICORE, "Created new CPU local storage at %p\n", this);
  MutexLock l(ArchMulticore::cpu_list_lock_);
  ArchMulticore::cpu_list_.push_back(this);
}

CpuLocalStorage* CpuLocalStorage::init()
{
  debug(A_MULTICORE, "Init CLS %p\n", this);
  cls_ptr = this;
  setGSBase((uint64)this);
  setSWAPGSKernelBase((uint64)this);
  return this;
}

size_t CpuLocalStorage::getCpuID()
{
        return cpu_id;
}

CpuLocalStorage* ArchMulticore::getCLS()
{
  CpuLocalStorage* cls_ptr;
  assert(ArchMulticore::CLSinitialized());
  __asm__ __volatile__("movq %%gs:0, %%rax\n"
                       "movq %%rax, %[cls_ptr]\n"
                       : [cls_ptr]"=m"(cls_ptr));
  assert(cls_ptr != 0);
  return cls_ptr;
}

CpuLocalStorage* ArchMulticore::initCLS()
{
  debug(A_MULTICORE, "Init CPU local storage\n");
  CpuLocalStorage* cls = new CpuLocalStorage{};
  setCpuID(LocalAPIC::exists && cls->apic.isInitialized()  ? cls->apic.getID() : 0);
  return getCLS();
}

bool ArchMulticore::CLSinitialized()
{
        return getGSBase() != 0;
}

void ArchMulticore::setCpuID(size_t id)
{
  getCLS()->cpu_id = id;
}

size_t ArchMulticore::getCpuID()
{
  return getCLS()->cpu_id;
}

extern uint8 boot_stack[0x4000];
extern SegmentDescriptor gdt[7];
struct GDT64Ptr
{
        uint16 limit;
        uint64 addr;
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

void ArchMulticore::initCpuLocalGDT(SegmentDescriptor* template_gdt)
{
  CpuLocalStorage* cls = getCLS();
  debug(A_MULTICORE, "CPU switching to own GDT at: %p\n", &cls->gdt);
  memcpy(&cls->gdt, template_gdt, sizeof(cls->gdt));

  struct GDT64Ptr gdt_ptr;
  gdt_ptr.limit = sizeof(cls->gdt) - 1;
  gdt_ptr.addr = (uint64)&cls->gdt;
  __asm__ __volatile__("lgdt %[gdt]\n"
                       "mov %%ax, %%ds\n"
                       "mov %%ax, %%es\n"
                       "mov %%ax, %%ss\n"
                       "mov %%ax, %%fs\n"
                       :
                       :[gdt]"m"(gdt_ptr), "a"(KERNEL_DS));
}

void ArchMulticore::initCpuLocalTSS(size_t boot_stack_top)
{
        CpuLocalStorage* cls = getCLS();
        debug(A_MULTICORE, "CPU init TSS at %p\n", &cls->tss);
        setTSSSegmentDescriptor((TSSSegmentDescriptor*)((char*)&cls->gdt + KERNEL_TSS), (size_t)&cls->tss >> 32, (size_t)&cls->tss, sizeof(TSS) - 1, 0);

        cls->tss.ist0 = boot_stack_top;
        cls->tss.rsp0 = boot_stack_top;
        __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));
}

void ArchMulticore::initialize()
{
  new (&cpu_list_) ustl::vector<CpuLocalStorage*>;
  new (&cpu_list_lock_) Mutex("CPU list lock");
  CpuLocalStorage* cls = ArchMulticore::initCLS();
  debug(A_MULTICORE, "CLS for cpu %zu at %p\n", ArchMulticore::getCpuID(), cls);

  initCpuLocalGDT(gdt);
  initCpuLocalTSS((size_t)&boot_stack + sizeof(boot_stack));
}

extern char apstartup_text_begin;
extern char apstartup_text_end;
extern "C" void apstartup();

struct GDT32Ptr
{
  uint16 limit;
  uint32 addr;
}__attribute__((__packed__));
extern GDT32Ptr ap_gdt32_ptr;

extern SegmentDescriptor ap_gdt32[7];

extern uint32 ap_kernel_cr3;
extern char ap_pml4[PAGE_SIZE];

void ArchMulticore::prepareAPStartup(size_t entry_addr)
{
  size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
  debug(A_MULTICORE, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n", &apstartup_text_begin, &apstartup_text_end, apstartup_size);

  debug(A_MULTICORE, "apstartup %p, phys: %zx\n", &apstartup, (size_t)VIRTUAL_TO_PHYSICAL_BOOT(entry_addr));

  pointer paddr0 = ArchMemory::getIdentAddress(entry_addr);
  auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), paddr0/PAGE_SIZE);
  assert(m.page); // TODO: Map if not present
  assert(m.page_ppn == entry_addr/PAGE_SIZE);

  memcpy(&ap_gdt32, &gdt, sizeof(ap_gdt32));
  ap_gdt32_ptr.addr = entry_addr + ((size_t)&ap_gdt32 - (size_t)&apstartup_text_begin);
  ap_gdt32_ptr.limit = sizeof(ap_gdt32) - 1;

  memcpy(&ap_pml4, &kernel_page_map_level_4, sizeof(ap_pml4));
  ap_kernel_cr3 = (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);

  debug(A_MULTICORE, "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n", (void*)&apstartup_text_begin, (void*)&apstartup_text_end, (void*)paddr0, (size_t)entry_addr, (size_t)(&apstartup_text_end - &apstartup_text_begin));
  memcpy((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size);
}

void ArchMulticore::startOtherCPUs()
{
  if(LocalAPIC::exists && getCLS()->apic.isInitialized())
  {
    debug(A_MULTICORE, "Starting other CPUs\n");

    prepareAPStartup(AP_STARTUP_PADDR);

    getCLS()->apic.startAPs(AP_STARTUP_PADDR);
    assert(otherCPUsStarted());
  }
  else
  {
    debug(A_MULTICORE, "No local APIC. Cannot start other CPUs\n");
  }
}

bool ArchMulticore::otherCPUsStarted()
{
  return cpus_started_;
}

void ArchMulticore::stopAllCpus()
{
  if(ArchMulticore::CLSinitialized() && ArchMulticore::getCLS()->apic.isInitialized())
  {
    getCLS()->apic.sendIPI(90);
  }
}


// TODO: Each AP needs its own stack
uint8 ap_stack[0x4000];


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
  ArchMulticore::initCpu();
}

void ArchMulticore::initCpu()
{
  debug(A_MULTICORE, "initAPCore\n");
  kprintf("initAPCore\n");

  debug(A_MULTICORE, "AP switching from temp kernel pml4 to main kernel pml4: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4));
  __asm__ __volatile__("movq %[kernel_cr3], %%cr3\n"
                       :
                       :[kernel_cr3]"a"(VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4)));

  ArchMulticore::initCLS();
  debug(A_MULTICORE, "getCLS(): %p, core id: %zx\n", ArchMulticore::getCLS(), ArchMulticore::getCpuID());

  initCpuLocalGDT(ap_gdt32);
  initCpuLocalTSS((size_t)&ap_stack + sizeof(ap_stack));

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::lidt(&InterruptUtils::idtr);

  debug(A_MULTICORE, "Init AP APIC\n");
  getCLS()->apic.init();
  ArchMulticore::setCpuID(getCLS()->apic.getID());

  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  ArchInterrupts::enableTimer();

  debug(A_MULTICORE, "Wait for system start\n");
  while(system_state != RUNNING);

  debug(A_MULTICORE, "Enabling interrupts\n");
  kprintf("Core %zu initialized, enabling interrupts\n", ArchMulticore::getCpuID());
  ArchInterrupts::enableInterrupts();

  while(1)
  {
    debug(A_MULTICORE, "AP %zu halting\n", ArchMulticore::getCpuID());
    __asm__ __volatile__("hlt\n");
  }
}
