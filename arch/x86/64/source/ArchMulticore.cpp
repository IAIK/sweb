#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"
#include "MutexLock.h"
#include "uatomic.h"
#include "Scheduler.h"
#include "ArchThreads.h"
#include "ArchCommon.h"

extern SystemState system_state;


__thread GDT cpu_gdt;
__thread TSS cpu_tss;

/* The order of initialization of thread_local objects depends on the order in which they are defined in the source code.
   This is pretty fragile, but using __thread and placement new doesn't work (compiler complains that dynamic initialization is required).
   Alternative: default constructor that does nothing + later explicit initialization using init() function */
thread_local CpuInfo cpu_info;

thread_local char cpu_stack[CPU_STACK_SIZE];


volatile static bool ap_started = false;


ustl::atomic<size_t> running_cpus;
ustl::vector<CpuInfo*> ArchMulticore::cpu_list_;
Mutex ArchMulticore::cpu_list_lock_("CPU list lock");

extern GDT32Ptr ap_gdt32_ptr;
extern GDT ap_gdt32;

extern GDT gdt;

extern char apstartup_text_begin;
extern char apstartup_text_end;
extern "C" void apstartup();

extern uint32 ap_kernel_cr3;
extern char ap_pml4[PAGE_SIZE];

static uint8 ap_boot_stack[PAGE_SIZE];

CpuInfo::CpuInfo() :
  lapic(),
  cpu_id(LocalAPIC::exists && lapic.isInitialized() ? lapic.ID() : 0)
{
  debug(A_MULTICORE, "Initializing CpuInfo %zx\n", cpu_id);
  MutexLock l(ArchMulticore::cpu_list_lock_);
  ArchMulticore::cpu_list_.push_back(this);
  debug(A_MULTICORE, "Added CpuInfo %zx to cpu list\n", cpu_id);
}

size_t CpuInfo::getCpuID()
{
  return cpu_id;
}

void CpuInfo::setCpuID(size_t id)
{
  cpu_id = id;
}



extern char cls_start;
extern char cls_end;
extern char tbss_start;
extern char tbss_end;
extern char tdata_start;
extern char tdata_end;

size_t CPULocalStorage::getCLSSize()
{
  return &cls_end - &cls_start;
}

char* CPULocalStorage::allocCLS()
{
  debug(A_MULTICORE, "Allocating CPU local storage\n");

  size_t cls_size = getCLSSize();
  size_t tbss_size = &tbss_end - &tbss_start;
  size_t tdata_size = &tdata_end - &tdata_start;
  debug(A_MULTICORE, "cls_base: [%p, %p), size: %zx\n", &cls_start, &cls_end, cls_size);
  debug(A_MULTICORE, "tbss: [%p, %p), size: %zx\n", &tbss_start, &tbss_end, tbss_size);
  debug(A_MULTICORE, "tdata: [%p, %p), size: %zx\n", &tdata_start, &tdata_end, tdata_size);

  char* cls_base = new char[cls_size + sizeof(void*)]{};
  debug(A_MULTICORE, "Allocated new cls_base at [%p, %p)\n", cls_base, cls_base + cls_size + sizeof(void*));

  debug(A_MULTICORE, "Initializing tdata at [%p, %p) and tbss at [%p, %p)\n",
        cls_base + (&tdata_start - &cls_start), cls_base + (&tdata_start - &cls_start) + tdata_size,
        cls_base + (&tbss_start - &cls_start), cls_base + (&tbss_start - &cls_start) + tbss_size);
  memcpy(cls_base + (&tdata_start - &cls_start), &tdata_start, tdata_size);

  return cls_base;
}

void CPULocalStorage::setCLS(char* cls)
{
  debug(A_MULTICORE, "Set CLS: %p\n", cls);
  void** fs_base = (void**)(cls + getCLSSize());
  *fs_base = fs_base;
  setFSBase((size_t)fs_base); // %fs base needs to point to end of CLS, not the start. %fs:0 = pointer to %fs base
  setGSBase((size_t)fs_base);
  setSWAPGSKernelBase((size_t)fs_base);

  debug(A_MULTICORE, "FS base: %p\n", (void*)getFSBase());
  debug(A_MULTICORE, "GS base: %p\n", (void*)getGSBase());
}

bool CPULocalStorage::CLSinitialized()
{
  bool init = (getFSBase() != 0);
  return init;
}




void ArchMulticore::initCPULocalData(bool boot_cpu)
{
  initCpuLocalGDT(boot_cpu ? gdt : ap_gdt32);
  initCpuLocalTSS((size_t)ArchMulticore::cpuStackTop());

  // The constructor of objects declared as thread_local will be called automatically the first time the thread_local object is used. Other thread_local objects _may or may not_ also be initialized at the same time.
  debug(A_MULTICORE, "Initializing CPU local objects for CPU %zx\n", cpu_info.getCpuID());
  // This is a dirty hack to make sure the idle thread is initialized. Otherwise idle thread initialization might happen the first time it gets scheduled, which won't work because it requires e.g. the KMM lock
  debug(A_MULTICORE, "CPU %zx: %s initialized\n", getCpuID(), idle_thread.getName());
}



void ArchMulticore::initCpuLocalGDT(GDT& template_gdt)
{
  cpu_gdt = template_gdt;

  debug(A_MULTICORE, "CPU switching to own GDT at: %p\n", &cpu_gdt);
  GDT64Ptr(cpu_gdt).load();
  __asm__ __volatile__("mov %%ax, %%ds\n"
                       "mov %%ax, %%es\n"
                       "mov %%ax, %%ss\n"
                       ::"a"(KERNEL_DS));
}

void ArchMulticore::initCpuLocalTSS(size_t cpu_stack_top)
{
  debug(A_MULTICORE, "CPU init TSS at %p\n", &cpu_tss);
  setTSSSegmentDescriptor((TSSSegmentDescriptor*)((char*)&cpu_gdt + KERNEL_TSS), (size_t)&cpu_tss >> 32, (size_t)&cpu_tss, sizeof(TSS) - 1, 0);

  cpu_tss.ist0 = cpu_stack_top;
  cpu_tss.rsp0 = cpu_stack_top;
  debug(A_MULTICORE, "Loading TSS\n");
  __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));
}

void ArchMulticore::setCpuID(size_t id)
{
  debug(A_MULTICORE, "Setting CPU ID %zu\n", id);
  assert(CPULocalStorage::CLSinitialized());
  cpu_info.setCpuID(id);
}

size_t ArchMulticore::getCpuID() // Only accurate when interrupts are disabled
{
  //assert(CLSinitialized());
  return (!CPULocalStorage::CLSinitialized() ? 0 : cpu_info.getCpuID());
}


void ArchMulticore::initialize()
{
  new (&cpu_list_) ustl::vector<CpuInfo*>;
  new (&cpu_list_lock_) Mutex("CPU list lock");

  assert(running_cpus == 0);
  running_cpus = 1;
  CPULocalStorage::setCLS(CPULocalStorage::allocCLS());
  ArchMulticore::initCPULocalData(true);
}

void ArchMulticore::prepareAPStartup(size_t entry_addr)
{
  size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
  debug(A_MULTICORE, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n", &apstartup_text_begin, &apstartup_text_end, apstartup_size);

  debug(A_MULTICORE, "apstartup %p, phys: %zx\n", &apstartup, (size_t)VIRTUAL_TO_PHYSICAL_BOOT(entry_addr));

  pointer paddr0 = ArchMemory::getIdentAddress(entry_addr);
  auto m = ArchMemory::resolveMapping(((size_t) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), paddr0/PAGE_SIZE);
  assert(m.page); // TODO: Map if not present
  assert(m.page_ppn == entry_addr/PAGE_SIZE);

  // Init AP gdt
  memcpy(&ap_gdt32, &gdt, sizeof(ap_gdt32));
  ap_gdt32_ptr.addr = entry_addr + ((size_t)&ap_gdt32 - (size_t)&apstartup_text_begin);
  ap_gdt32_ptr.limit = sizeof(ap_gdt32) - 1;

  // Init AP PML4
  memcpy(&ap_pml4, &kernel_page_map_level_4, sizeof(ap_pml4));
  ap_kernel_cr3 = (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);

  debug(A_MULTICORE, "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n", (void*)&apstartup_text_begin, (void*)&apstartup_text_end, (void*)paddr0, (size_t)entry_addr, (size_t)(&apstartup_text_end - &apstartup_text_begin));
  memcpy((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size);
}

void ArchMulticore::startOtherCPUs()
{
  if(LocalAPIC::exists && cpu_info.lapic.isInitialized())
  {
    debug(A_MULTICORE, "Starting other CPUs\n");

    prepareAPStartup(AP_STARTUP_PADDR);

    for(auto& cpu_lapic : LocalAPIC::local_apic_list_)
    {
      if(cpu_lapic.flags.enabled && (cpu_lapic.apic_id != cpu_info.lapic.ID()))
      {
        cpu_info.lapic.startAP(cpu_lapic.apic_id, AP_STARTUP_PADDR);
        debug(A_MULTICORE, "BSP waiting for AP %x startup to be complete\n", cpu_lapic.apic_id);
        while(!ap_started);
        ap_started = false;
        debug(A_MULTICORE, "AP %u startup complete, BSP continuing\n", cpu_lapic.apic_id);
      }
    }

    MutexLock l(ArchMulticore::cpu_list_lock_);
    for(auto& cpu : ArchMulticore::cpu_list_)
    {
      debug(A_MULTICORE, "CPU %zu running\n", cpu->getCpuID());
    }
  }
  else
  {
    debug(A_MULTICORE, "No local APIC. Cannot start other CPUs\n");
  }
}

size_t ArchMulticore::numRunningCPUs()
{
  return running_cpus;
}

void ArchMulticore::stopAllCpus()
{
  if(CPULocalStorage::CLSinitialized() && cpu_info.lapic.isInitialized())
  {
    cpu_info.lapic.sendIPI(90);
  }
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
                       :[stack]"i"(ap_boot_stack + sizeof(ap_boot_stack)));

  ++running_cpus;
  debug(A_MULTICORE, "AP startup 64\n");
  debug(A_MULTICORE, "AP switched to stack %p\n", ap_boot_stack + sizeof(ap_boot_stack));

  // Stack variables are messed up in this function because we skipped the function prologue. Should be fine once we've entered another function.
  ArchMulticore::initCpu();
}

void ArchMulticore::initCpu()
{
  debug(A_MULTICORE, "AP switching from temp kernel page tables to main kernel page tables: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4));
  ArchMemory::loadPagingStructureRoot((size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()));

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::idtr.load();

  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for AP [%p, %p)\n", &cls_start, &cls_end);
  CPULocalStorage::setCLS(&cls_start);
  currentThread = NULL;

  CPULocalStorage::setCLS(CPULocalStorage::allocCLS());
  ArchMulticore::initCPULocalData();
  cpu_info.lapic.init();

  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  ArchInterrupts::enableTimer();

  debug(A_MULTICORE, "Switching to CPU local stack at %p\n", ArchMulticore::cpuStackTop());
  __asm__ __volatile__("movq %[cpu_stack], %%rsp\n"
                       "movq %%rsp, %%rbp\n"
                       ::[cpu_stack]"r"(ArchMulticore::cpuStackTop()));
  waitForSystemStart();
}


void ArchMulticore::waitForSystemStart()
{
  kprintf("CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
  debug(A_MULTICORE, "CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
  assert(CPULocalStorage::CLSinitialized());
  ap_started = true;

  while(system_state != RUNNING);

  //debug(A_MULTICORE, "CPU %zu enabling interrupts\n", ArchMulticore::getCpuID());
  ArchInterrupts::enableInterrupts();

  while(1)
  {
    debug(A_MULTICORE, "AP %zu halting\n", ArchMulticore::getCpuID());
    ArchCommon::halt();
  }
  assert(false);
}


char* ArchMulticore::cpuStackTop()
{
  return cpu_stack + sizeof(cpu_stack);
}
