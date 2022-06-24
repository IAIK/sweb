#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "ArchCommon.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Thread.h"
#include "MutexLock.h"
#include "EASTL/atomic.h"
#include "Scheduler.h"
#include "ArchThreads.h"
#include "ArchCommon.h"

extern SystemState system_state;


__thread GDT cpu_gdt;
__thread TSS cpu_tss;

/* The order of initialization of thread_local objects depends on the order in which they are defined in the source code.
   This is pretty fragile, but using __thread and placement new doesn't work (compiler complains that dynamic initialization is required).
   Alternative: default constructor that does nothing + later explicit initialization using init() function */

thread_local LocalAPIC cpu_lapic;
thread_local size_t cpu_id;
thread_local CpuInfo cpu_info;

thread_local char cpu_stack[CPU_STACK_SIZE];


volatile static bool ap_started = false; // TODO: Convert to spinlock


eastl::atomic<size_t> running_cpus;
eastl::vector<CpuInfo*> ArchMulticore::cpu_list_;
Mutex ArchMulticore::cpu_list_lock_("CPU list lock");

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

CpuInfo::CpuInfo() :
  lapic(&cpu_lapic),
  cpu_id_(&cpu_id)
{
  setCpuID(LocalAPIC::exists && lapic->isInitialized() ? lapic->ID() : 0);
  debug(A_MULTICORE, "Initializing CpuInfo for CPU %zx at %p\n", cpu_id, this);
  ArchMulticore::addCPUtoList(this);
}


size_t CpuInfo::getCpuID()
{
  return *cpu_id_;
}

void CpuInfo::setCpuID(size_t id)
{
  *cpu_id_ = id;
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

void CPULocalStorage::setCLS(GDT& gdt, char* cls)
{
  debug(A_MULTICORE, "Set CLS: %p\n", cls);
  void** gs_base = (void**)(cls + getCLSSize());
  debug(A_MULTICORE, "Init CLS pointer at %%gs:0 = %p\n", gs_base);
  *gs_base = gs_base;

  // %gs base needs to point to end of CLS, not the start. %gs:0 = pointer to %gs base
  setGSBase(gdt, (size_t)gs_base);
  setFSBase(gdt, (size_t)gs_base);

  debug(A_MULTICORE, "FS base: %p\n", (void*)getFSBase(gdt));
  debug(A_MULTICORE, "GS base: %p\n", (void*)getGSBase(gdt));
}

bool CPULocalStorage::CLSinitialized()
{
  uint32_t gs_val = 0;
  asm("mov %%gs, %[gs]\n"
      :[gs]"=g"(gs_val));

  return gs_val == KERNEL_GS;
}

void* CPULocalStorage::getClsBase()
{
  void *gs_base = 0;
  asm("movl %%gs:0, %[gs_base]\n" : [gs_base] "=r"(gs_base));
  return gs_base;
}

void ArchMulticore::initCPULocalData(bool boot_cpu)
{
  initCpuLocalGDT(boot_cpu ? gdt : ap_gdt32);
  initCpuLocalTSS((size_t)ArchMulticore::cpuStackTop());


  // The constructor of objects declared as thread_local will be called automatically the first time the thread_local object is used. Other thread_local objects _may or may not_ also be initialized at the same time.
  debug(A_MULTICORE, "Initializing CPU local objects for CPU %zu\n", cpu_info.getCpuID());

  // void* cpu_info_addr = &cpu_info;
  // debug(A_MULTICORE, "Initializing cpu_info at %p\n", &cpu_info_addr);
  // debug(A_MULTICORE, "Initializing CPU local objects for CPU %zx\n", cpu_info.getCpuID());

  idle_thread = new IdleThread();
  debug(A_MULTICORE, "CPU %zu: %s initialized\n", getCpuID(), idle_thread->getName());
  idle_thread->pinned_to_cpu = getCpuID();
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
  debug(A_MULTICORE, "Init cpu list at %p\n", &cpu_list_);
  new (&cpu_list_) eastl::vector<CpuInfo*>{};
  new (&cpu_list_lock_) Mutex("CPU list lock");

  assert(running_cpus == 0);
  running_cpus = 1;

  char *cls = CPULocalStorage::allocCLS();
  CPULocalStorage::setCLS(gdt, cls);
  ArchMulticore::initCPULocalData(true);
}

void ArchMulticore::addCPUtoList(CpuInfo* cpu)
{
        debug(A_MULTICORE, "Adding CpuInfo %zx to cpu list\n", cpu->getCpuID());
        MutexLock l(ArchMulticore::cpu_list_lock_);
        debug(A_MULTICORE, "Locked cpu list, list at %p\n", &ArchMulticore::cpu_list_);
        ArchMulticore::cpu_list_.push_back(cpu);
        debug(A_MULTICORE, "Added CpuInfo %zx to cpu list\n", cpu->getCpuID());
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
  memcpy((void*)ap_pml4_load_addr, ArchMemory::getRootOfKernelPagingStructure(), (size_t)(&ap_paging_root_end - &ap_paging_root));

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
  if(LocalAPIC::exists && cpu_lapic.isInitialized())
  {
    debug(A_MULTICORE, "Starting other CPUs\n");

    prepareAPStartup(AP_STARTUP_PADDR);

    for(auto& other_cpu_lapic : LocalAPIC::local_apic_list_)
    {
      if(other_cpu_lapic.flags.enabled && (other_cpu_lapic.apic_id != cpu_lapic.ID()))
      {
        cpu_lapic.startAP(other_cpu_lapic.apic_id, AP_STARTUP_PADDR);
        debug(A_MULTICORE, "BSP waiting for AP %x startup to be complete\n", other_cpu_lapic.apic_id);
        while(!ap_started);
        ap_started = false;
        debug(A_MULTICORE, "AP %u startup complete, BSP continuing\n", other_cpu_lapic.apic_id);
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
  if(CPULocalStorage::CLSinitialized() && cpu_lapic.isInitialized())
  {
    cpu_lapic.sendIPI(90);
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
      : [K_DS] "i"(KERNEL_DS),
        [stack] "i"(ap_boot_stack +
                    sizeof(ap_boot_stack)));

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
  debug(A_MULTICORE, "AP switching from temp kernel paging root to main kernel paging root: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()));
  ArchMemory::loadPagingStructureRoot(kernel_arch_mem.getValueForCR3());

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::idtr.load();

  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for AP [%p, %p)\n", &cls_start, &cls_end);
  CPULocalStorage::setCLS(ap_gdt32, &cls_start);
  currentThread = nullptr;
  currentThreadRegisters = nullptr;

  char* cls = CPULocalStorage::allocCLS();
  CPULocalStorage::setCLS(ap_gdt32, cls);

  cpu_lapic.init();
  cpu_info.setCpuID(cpu_lapic.readID());
  ArchMulticore::initCPULocalData();

  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  ArchInterrupts::enableTimer();

  debug(A_MULTICORE, "Switching to CPU local stack at %p\n", ArchMulticore::cpuStackTop());
  ArchCommon::callWithStack(ArchMulticore::cpuStackTop(), waitForSystemStart);
  waitForSystemStart();
}


[[noreturn]] void ArchMulticore::waitForSystemStart()
{
  kprintf("CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
  debug(A_MULTICORE, "CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
  assert(CPULocalStorage::CLSinitialized());
  ap_started = true;

  while(system_state != RUNNING);

  debug(A_MULTICORE, "CPU %zu enabling interrupts\n", ArchMulticore::getCpuID());
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
