#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "ArchCommon.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"
#include "ScopeLock.h"
#include "EASTL/atomic.h"
#include "Scheduler.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "Allocator.h"
#include "SystemState.h"
#include "ArchCpuLocalStorage.h"


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
extern char apstartup_text_load_begin;
extern char apstartup_text_load_end;
// extern "C" void apstartup();

extern uint32 ap_kernel_cr3;

extern char ap_paging_root;
extern char ap_paging_root_end;

static uint8 ap_boot_stack[PAGE_SIZE];


void ArchMulticore::initCpuLocalData(bool boot_cpu)
{
  initCpuLocalGDT(boot_cpu ? gdt : ap_gdt32);
  initCpuLocalTSS((size_t)ArchMulticore::cpuStackTop());


  // The constructor of objects declared as cpu_local will be called automatically the first time the cpu_local object is used. Other cpu_local objects _may or may not_ also be initialized at the same time.
  debug(A_MULTICORE, "Initializing CPU local objects for CPU %zu\n", SMP::currentCpuId());

  // void* cpu_info_addr = &cpu_info;
  // debug(A_MULTICORE, "Initializing cpu_info at %p\n", &cpu_info_addr);
  // debug(A_MULTICORE, "Initializing CPU local objects for CPU %zx\n", cpu_info.getCpuID());

  idle_thread = new IdleThread();
  debug(A_MULTICORE, "CPU %zu: %s initialized\n", SMP::currentCpuId(), idle_thread->getName());
  idle_thread->pinned_to_cpu = SMP::currentCpuId();
  Scheduler::instance()->addNewThread(idle_thread);
}



void ArchMulticore::initCpuLocalGDT(GDT& template_gdt)
{
  cpu_gdt = template_gdt;

  debug(A_MULTICORE, "CPU switching to own GDT at: %p\n", &cpu_gdt);
  GDT32Ptr(cpu_gdt).load();
  __asm__ __volatile__("mov %[ds], %%ds\n"
                       "mov %[ds], %%es\n"
                       "mov %[ds], %%ss\n"
                       "ljmp %[cs], $1f\n"
                       "1:\n"
                       :
                       :[ds]"a"(KERNEL_DS),
                        [cs]"i"(KERNEL_CS));
}

void ArchMulticore::initCpuLocalTSS(size_t cpu_stack_top)
{
  debug(A_MULTICORE, "CPU init TSS at %p\n", &cpu_tss);
  setTSSSegmentDescriptor((TSSSegmentDescriptor*)((char*)&cpu_gdt + KERNEL_TSS), (size_t)&cpu_tss, sizeof(TSS) - 1, 0);

  cpu_tss.setTaskStack(cpu_stack_top);

  __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));
}


void ArchMulticore::initialize()
{
  assert(running_cpus == 0);
  running_cpus = 1;

  char *cls = CpuLocalStorage::allocCls();
  CpuLocalStorage::setCls(gdt, cls);
  ArchMulticore::initCpuLocalData(true);
}

void ArchMulticore::prepareAPStartup(size_t entry_addr)
{
  size_t apstartup_size = (size_t)(&apstartup_text_load_end - &apstartup_text_load_begin);
  debug(A_MULTICORE, "text.apstartup begin: %p, text.apstartup end: %p\n",
        &apstartup_text_begin, &apstartup_text_end);
  debug(A_MULTICORE, "text.apstartup load begin: %p, text.apstartup load end: %p, size: %zx\n",
        &apstartup_text_load_begin, &apstartup_text_load_end, apstartup_size);
  debug(A_MULTICORE, "apstartup %p, phys: %p\n", &apstartup_text_load_begin, (void*)entry_addr);

  auto m_load = kernel_arch_mem.resolveMapping((size_t)&apstartup_text_load_begin/PAGE_SIZE);
  debug(A_MULTICORE, "apstartup load mapping %p: page: %p, ppn: %x, pt: %p, writeable: %u\n",
        &apstartup_text_load_begin, (void*)m_load.page, m_load.page_ppn, m_load.pt, m_load.pt[m_load.pti].writeable);
  assert(m_load.pt[m_load.pti].writeable);

  pointer paddr0 = ArchMemory::getIdentAddress(entry_addr);
  debug(A_MULTICORE, "Ident mapping for entry addr %x: %x\n", entry_addr, paddr0);
  auto m = kernel_arch_mem.resolveMapping(paddr0/PAGE_SIZE);

  assert(m.page && "Page for application processor entry not mapped in kernel"); // TODO: Map if not present
  debug(A_MULTICORE, "PPN: %x\n", m.page_ppn);
  assert((m.page_ppn == entry_addr/PAGE_SIZE) && "PPN in ident mapping doesn't match expected ppn for AP entry");

  size_t ap_gdt32_offset = (size_t)&ap_gdt32 - (size_t)&apstartup_text_begin;
  size_t ap_gdt32_load_addr = (size_t)&apstartup_text_load_begin + ap_gdt32_offset;
  debug(A_MULTICORE, "AP GDT load offset: %zx\n", ap_gdt32_offset);

  // Init AP gdt
  debug(A_MULTICORE, "Init AP GDT at %p, (loaded at %zx)\n", &ap_gdt32, ap_gdt32_load_addr);
  auto m_ap_gdt = kernel_arch_mem.resolveMapping(((size_t)&ap_gdt32_load_addr)/PAGE_SIZE);
  assert(m_ap_gdt.page && "AP GDT virtual address not mapped in kernel");
  assert(m_ap_gdt.pt && m_ap_gdt.pt[m_ap_gdt.pti].writeable && "AP GDT virtual address not writeable");

  memcpy((void*)ap_gdt32_load_addr, &gdt, sizeof(ap_gdt32));

  size_t ap_gdt32_ptr_offset = (size_t)&ap_gdt32_ptr - (size_t)&apstartup_text_begin;
  size_t ap_gdt32_ptr_load_addr = (size_t)&apstartup_text_load_begin + ap_gdt32_ptr_offset;
  GDT32Ptr *ap_gdt32_ptr_load = (GDT32Ptr*)ap_gdt32_ptr_load_addr;

  ap_gdt32_ptr_load->addr = (size_t)&ap_gdt32;
  ap_gdt32_ptr_load->limit = sizeof(ap_gdt32) - 1;

  debug(A_MULTICORE, "paddr0: %x\n", paddr0);

  // Init AP PD
  size_t ap_pml4_offset = (size_t)&ap_paging_root - (size_t)&apstartup_text_begin;
  size_t ap_pml4_load_addr = (size_t)&apstartup_text_load_begin + ap_pml4_offset;
  debug(A_MULTICORE, "Init AP PD at %p (loaded at %zx)\n", &ap_paging_root, ap_pml4_load_addr);
  memcpy((void*)ap_pml4_load_addr, ArchMemory::getKernelPagingStructureRootVirt(), (size_t)(&ap_paging_root_end - &ap_paging_root));

  debug(A_MULTICORE, "paddr0: %x\n", paddr0);
  debug(A_MULTICORE, "AP PD phys: %x\n", (size_t)&ap_paging_root);

  size_t ap_kernel_cr3_offset = (size_t)&ap_kernel_cr3 - (size_t)&apstartup_text_begin;
  size_t ap_kernel_cr3_load_addr = (size_t)&apstartup_text_load_begin + ap_kernel_cr3_offset;

  *(size_t*)ap_kernel_cr3_load_addr = (size_t)&ap_paging_root;

  debug(
      A_MULTICORE,
      "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n",
      (void *)&apstartup_text_load_begin, (void *)&apstartup_text_load_end,
      (void *)paddr0, (size_t)entry_addr, apstartup_size);
  memcpy((void*)paddr0, (void*)&apstartup_text_load_begin, apstartup_size);
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

extern "C" void __apstartup32() {
  // Hack to avoid automatic function prologue (stack isn't set up yet)
  // Load protected mode segments
  __asm__ __volatile__(
      ".global apstartup32\n"
      "apstartup32:\n"
      "movw %[K_DS], %%ax\n"
      "movw %%ax, %%ds\n"
      "movw %%ax, %%ss\n"
      "movw %%ax, %%es\n"
      "movw %%ax, %%fs\n"
      "movw %%ax, %%gs\n"
      "movl %[stack], %%esp\n"
      "movl %[stack], %%ebp\n"
      :
      : [K_DS]"i"(KERNEL_DS),
        [stack]"i"(ap_boot_stack + sizeof(ap_boot_stack)));

  ArchCommon::callWithStack((char *)ap_boot_stack + sizeof(ap_boot_stack), [] {
    ++running_cpus;
    debug(A_MULTICORE, "AP startup 32\n");
    debug(A_MULTICORE, "AP switched to stack %p\n",
          ap_boot_stack + sizeof(ap_boot_stack));

    // Enable NX bit
    if (PAGE_DIRECTORY_ENTRIES == 512)
    {
        asm("mov $0xC0000080,%ecx\n"
            "rdmsr\n"
            "or $0x800,%eax\n"
            "wrmsr\n");
    }

    // Stack variables are messed up in this function because we skipped the
    // function prologue. Should be fine once we've entered another function.
    ArchMulticore::initApplicationProcessorCpu();
  });
}

void ArchMulticore::initApplicationProcessorCpu()
{
  debug(A_MULTICORE, "AP switching from temp kernel paging root to main kernel paging root: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getKernelPagingStructureRootVirt()));
  ArchMemory::loadPagingStructureRoot(kernel_arch_mem.getValueForCR3());

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::idtr.load();

  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for AP [%p, %p)\n", &cls_start, &cls_end);
  CpuLocalStorage::setCls(ap_gdt32, &cls_start);
  currentThread = nullptr;
  currentThreadRegisters = nullptr;

  char* cls = CpuLocalStorage::allocCls();
  CpuLocalStorage::setCls(ap_gdt32, cls);

  ApicDriver::instance().cpuLocalInit();
  ApicTimerDriver::instance().cpuLocalInit();

  // cpu_lapic->init();
  // current_cpu.setId(cpu_lapic.readID());
  assert(cpu_lapic->apicId() == CPUID::localApicId());
  ArchMulticore::initCpuLocalData();

  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  assert(cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer() &&
         "Use of local APIC timer is required for SMP");
  ArchInterrupts::enableTimer();

  debug(A_MULTICORE, "Switching to CPU local stack at %p\n", ArchMulticore::cpuStackTop());
  ArchCommon::callWithStack(ArchMulticore::cpuStackTop(), waitForSystemStart);
  waitForSystemStart();
}
