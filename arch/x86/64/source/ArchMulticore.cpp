#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"
#include "ScopeLock.h"
#include "EASTL/atomic.h"
#include "Scheduler.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "Allocator.h"
#include "assert.h"
#include "SystemState.h"
#include "CPUID.h"
#include "PlatformBus.h"


cpu_local GDT cpu_gdt;
cpu_local TSS cpu_tss;

cpu_local XApic cpu_lapic_impl;
cpu_local Apic* cpu_lapic = &cpu_lapic_impl;

cpu_local char cpu_stack[CPU_STACK_SIZE];


eastl::atomic<bool> ap_started = false;


extern eastl::atomic<size_t> running_cpus;

extern GDT32Ptr ap_gdt32_ptr;
extern GDT ap_gdt32;

extern GDT gdt;

extern char apstartup_text_begin;
extern char apstartup_text_end;
extern "C" void apstartup();

extern uint32 ap_kernel_cr3;
extern char ap_pml4[PAGE_SIZE];

static uint8 ap_boot_stack[PAGE_SIZE];


void ArchMulticore::initCpuLocalData(bool boot_cpu)
{
  initCpuLocalGDT(boot_cpu ? gdt : ap_gdt32);
  initCpuLocalTSS((size_t)ArchMulticore::cpuStackTop());

  // The constructor of objects declared as cpu_local will be called automatically the first time the cpu_local object is used. Other cpu_local objects _may or may not_ also be initialized at the same time.
  debug(A_MULTICORE, "Initializing CPU local objects for CPU %zu\n", SMP::currentCpuId());

  idle_thread = new IdleThread();
  debug(A_MULTICORE, "CPU %zu: %s initialized\n", SMP::currentCpuId(), idle_thread->getName());
  idle_thread->pinned_to_cpu = SMP::currentCpuId();
  Scheduler::instance()->addNewThread(idle_thread);
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

  cpu_tss.setTaskStack(cpu_stack_top);

  __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));
}

void ArchMulticore::initialize()
{
  assert(running_cpus == 0);
  running_cpus = 1;

  CpuLocalStorage::initCpuLocalStorage();
  ArchMulticore::initCpuLocalData(true);
}

void ArchMulticore::prepareAPStartup(size_t entry_addr)
{
  size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
  debug(A_MULTICORE, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n",
        &apstartup_text_begin, &apstartup_text_end, apstartup_size);

  debug(A_MULTICORE, "apstartup %p, phys: %zx\n", &apstartup, (size_t)VIRTUAL_TO_PHYSICAL_BOOT(entry_addr));


  pointer paddr0 = ArchMemory::getIdentAddressOfPPN(entry_addr/PAGE_SIZE) + (entry_addr % PAGE_SIZE);
  auto m = ArchMemory::kernelArchMemory().resolveMapping(paddr0/PAGE_SIZE);
  assert(m.page && "Page for application processor entry not mapped in kernel"); // TODO: Map if not present
  assert((m.page_ppn == entry_addr/PAGE_SIZE) && "PPN in ident mapping doesn't match expected ppn for AP entry");

  // Init AP gdt
  debug(A_MULTICORE, "Init AP GDT at %p\n", &ap_gdt32);
  auto m_ap_gdt = ArchMemory::kernelArchMemory().resolveMapping(((size_t)&ap_gdt32)/PAGE_SIZE);
  assert(m_ap_gdt.page && "AP GDT virtual address not mapped in kernel");
  debug(A_MULTICORE, "AP GDT mapped on ppn %#lx\n", m_ap_gdt.page_ppn);

  memcpy(&ap_gdt32, &gdt, sizeof(ap_gdt32));
  ap_gdt32_ptr.addr = entry_addr + ((size_t)&ap_gdt32 - (size_t)&apstartup_text_begin);
  ap_gdt32_ptr.limit = sizeof(ap_gdt32) - 1;

  // Init AP PML4
  debug(A_MULTICORE, "Init AP PML4\n");
  memcpy(&ap_pml4, &kernel_page_map_level_4, sizeof(ap_pml4));
  ap_kernel_cr3 = (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4); // TODO: shouldn't this be ap_pml4?

  debug(A_MULTICORE, "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n", (void*)&apstartup_text_begin, (void*)&apstartup_text_end, (void*)paddr0, (size_t)entry_addr, (size_t)(&apstartup_text_end - &apstartup_text_begin));
  memcpy((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size);
}

void ArchMulticore::startOtherCPUs()
{
  if(cpu_lapic->isInitialized())
  {
    debug(A_MULTICORE, "Starting other CPUs\n");

    prepareAPStartup(AP_STARTUP_PADDR);

    for(auto& other_cpu_lapic : Apic::local_apic_list_)
    {
      if(other_cpu_lapic.flags.enabled && (other_cpu_lapic.apic_id != cpu_lapic->apicId()))
      {
        cpu_lapic->startAP(other_cpu_lapic.apic_id, AP_STARTUP_PADDR);
        debug(A_MULTICORE, "BSP waiting for AP %x startup to be complete\n", other_cpu_lapic.apic_id);
        while(!ap_started);
        ap_started = false;
        debug(A_MULTICORE, "AP %u startup complete, BSP continuing\n", other_cpu_lapic.apic_id);
      }
    }

    ScopeLock l(SMP::cpu_list_lock_);
    for(auto& cpu : SMP::cpuList())
    {
      debug(A_MULTICORE, "CPU %zu running\n", cpu->id());
    }
  }
  else
  {
    debug(A_MULTICORE, "No local APIC. Cannot start other CPUs\n");
  }
}

extern "C" __attribute__((naked)) void apstartup64()
{
  // Stack is not set up yet, no function prologue!
  // Load long mode data segments
  __asm__ __volatile__(
    "movw %[K_DS], %%ax\n"
    "movw %%ax, %%ds\n"
    "movw %%ax, %%ss\n"
    "movw %%ax, %%es\n"
    "movw %%ax, %%fs\n"
    "movw %%ax, %%gs\n"
    "movq %[stack], %%rsp\n"
    "movq %[stack], %%rbp\n"
    :
    : [K_DS]"i"(KERNEL_DS),
      [stack]"i"(ap_boot_stack + sizeof(ap_boot_stack)));

  ++running_cpus;
  debug(A_MULTICORE, "AP startup 64\n");
  debug(A_MULTICORE, "AP switched to stack %p\n", ap_boot_stack + sizeof(ap_boot_stack));

  // Stack variables are messed up in this function because we skipped the function prologue. Should be fine once we've entered another function.
  ArchMulticore::initCpu();
}

extern void initCpuLocalInterruptHandlers();

void ArchMulticore::initCpu()
{
  debug(A_MULTICORE, "AP switching from temp kernel page tables to main kernel page tables: %zx\n", (size_t)ArchMemory::kernelArchMemory().getPagingStructureRootPhys());
  ArchMemory::loadPagingStructureRoot(ArchMemory::kernelArchMemory().getValueForCR3());

  InterruptUtils::idt.idtr().load();

  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for AP [%p, %p)\n", &cls_start, &cls_end);
  CpuLocalStorage::setCls(&cls_start);
  currentThread = nullptr;

  CpuLocalStorage::initCpuLocalStorage();

  ApicDriver::instance().cpuLocalInit();
  ApicTimerDriver::instance().cpuLocalInit();

  initCpuLocalInterruptHandlers();

  assert(cpu_lapic->apicId() == CPUID::localApicId());
  ArchMulticore::initCpuLocalData();

  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  assert(cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer() && "Use of local APIC timer is required for SMP");
  ArchInterrupts::enableTimer();

  debug(A_MULTICORE, "Switching to CPU local stack at %p\n", ArchMulticore::cpuStackTop());
  ArchCommon::callWithStack(ArchMulticore::cpuStackTop(), waitForSystemStart);
}
