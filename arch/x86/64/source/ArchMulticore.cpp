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

__thread GDT cpu_gdt;
__thread TSS cpu_tss;

/* The order of initialization of thread_local objects depends on the order in which they are defined in the source code.
   This is pretty fragile, but using __thread and placement new doesn't work (compiler complains that dynamic initialization is required).
   Alternative: default constructor that does nothing + later explicit initialization using init() function */
thread_local CpuInfo cpu_info;

thread_local char cpu_stack[CPU_STACK_SIZE];

extern SystemState system_state;

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


#define MSR_FS_BASE        0xC0000100
#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

void getMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
        asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void setMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
        if(A_MULTICORE & OUTPUT_ADVANCED)
        {
                debug(A_MULTICORE, "Set MSR %x, value: %zx\n", msr, ((size_t)hi << 32) | (size_t)lo);
        }
        asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

uint64 ArchMulticore::getGSBase()
{
        uint64 gs_base;
        getMSR(MSR_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
        return gs_base;
}

uint64 ArchMulticore::getGSKernelBase()
{
        uint64 gs_base;
        getMSR(MSR_KERNEL_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
        return gs_base;
}

uint64 ArchMulticore::getFSBase()
{
        uint64 fs_base;
        getMSR(MSR_FS_BASE, (uint32*)&fs_base, ((uint32*)&fs_base) + 1);
        return fs_base;
}

void ArchMulticore::setGSBase(uint64 gs_base)
{
        setMSR(MSR_GS_BASE, gs_base, gs_base >> 32);
}

void ArchMulticore::setFSBase(uint64 fs_base)
{
        setMSR(MSR_FS_BASE, fs_base, fs_base >> 32);
}

void setSWAPGSKernelBase(uint64 swapgs_base)
{
        setMSR(MSR_KERNEL_GS_BASE, swapgs_base, swapgs_base >> 32);
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

CpuInfo::CpuInfo() :
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

void* ArchMulticore::getSavedFSBase()
{
  void* fs_base;
  __asm__ __volatile__("movq %%gs:0, %%rax\n"
                       "movq %%rax, %[fs_base]\n"
                       : [fs_base]"=m"(fs_base));
  assert(fs_base != 0);
  return fs_base;
}

extern char cls_start;
extern char cls_end;
extern char tbss_start;
extern char tbss_end;
extern char tdata_start;
extern char tdata_end;

void ArchMulticore::allocCLS(char*& cls, size_t& cls_size)
{
  debug(A_MULTICORE, "Allocating CPU local storage\n");
  cls_size = &cls_end - &cls_start;
  size_t tbss_size = &tbss_end - &tbss_start;
  size_t tdata_size = &tdata_end - &tdata_start;
  debug(A_MULTICORE, "cls: [%p, %p), size: %zx\n", &cls_start, &cls_end, cls_size);
  debug(A_MULTICORE, "tbss: [%p, %p), size: %zx\n", &tbss_start, &tbss_end, tbss_size);
  debug(A_MULTICORE, "tdata: [%p, %p), size: %zx\n", &tdata_start, &tdata_end, tdata_size);

  cls = new char[cls_size + sizeof(void*)]{};
  debug(A_MULTICORE, "Allocated new cls at [%p, %p)\n", cls, cls + cls_size + sizeof(void*));

  debug(A_MULTICORE, "Initializing tdata at [%p, %p) and tbss at [%p, %p)\n", cls + (&tdata_start - &cls_start), cls + (&tdata_start - &cls_start) + tdata_size, cls + (&tbss_start - &cls_start), cls + (&tbss_start - &cls_start) + tbss_size);
  memcpy(cls + (&tdata_start - &cls_start), &tdata_start, tdata_size);
}

void ArchMulticore::setCLS(char* cls, size_t cls_size)
{
        debug(A_MULTICORE, "Set CLS: %p, size: %zx\n", cls, cls_size);
        void** fs_base = (void**)(cls + cls_size);
        *fs_base = fs_base;
        setFSBase((uint64)fs_base); // %fs base needs to point to end of CLS, not the start. %fs:0 = pointer to %fs base
        setGSBase((uint64)fs_base);
        setSWAPGSKernelBase((uint64)fs_base);

        debug(A_MULTICORE, "FS base: %p\n", (void*)getFSBase());
        debug(A_MULTICORE, "GS base: %p\n", (void*)getGSBase());
}

void ArchMulticore::initCLS(bool boot_cpu)
{
  char* cls = 0;
  size_t cls_size = 0;


  allocCLS(cls, cls_size);
  setCLS(cls, cls_size);

  initCpuLocalGDT(boot_cpu ? gdt : ap_gdt32);
  initCpuLocalTSS((size_t)(cpu_stack + sizeof(cpu_stack)));

  // The constructor of objects declared as thread_local will be called automatically the first time the thread_local object is used. Other thread_local objects _may or may not_ also be initialized at the same time.
  debug(A_MULTICORE, "Initializing CPU local objects for CPU %zx\n", cpu_info.getCpuID());
  // This is a dirty hack to make sure the idle thread is initialized. Otherwise idle thread initialization might happen the first time it gets scheduled, which won't work because it requires e.g. the KMM lock
  debug(A_MULTICORE, "CPU %zx: %s initialized\n", getCpuID(), idle_thread.getName());
}

bool ArchMulticore::CLSinitialized()
{
  return getFSBase() != 0;
}

void ArchMulticore::setCpuID(size_t id)
{
  debug(A_MULTICORE, "Setting CPU ID %zu\n", id);
  cpu_info.cpu_id = id;
}

size_t ArchMulticore::getCpuID()
{
  assert(CLSinitialized());
  return cpu_info.getCpuID();
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
        __asm__ __volatile__("ltr %%ax" : : "a"(KERNEL_TSS));
}

void ArchMulticore::initialize()
{
  new (&cpu_list_) ustl::vector<CpuInfo*>;
  new (&cpu_list_lock_) Mutex("CPU list lock");

  assert(running_cpus == 0);
  running_cpus = 1;
  ArchMulticore::initCLS(true);
}

void ArchMulticore::prepareAPStartup(size_t entry_addr)
{
  size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
  debug(A_MULTICORE, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n", &apstartup_text_begin, &apstartup_text_end, apstartup_size);

  debug(A_MULTICORE, "apstartup %p, phys: %zx\n", &apstartup, (size_t)VIRTUAL_TO_PHYSICAL_BOOT(entry_addr));

  pointer paddr0 = ArchMemory::getIdentAddress(entry_addr);
  auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), paddr0/PAGE_SIZE);
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
                    debug(A_MULTICORE, "AP %x startup complete, BSP continuing\n", cpu_lapic.apic_id);
            }
    }

    MutexLock l(ArchMulticore::cpu_list_lock_);
    for(auto& cpu : ArchMulticore::cpu_list_)
    {
            debug(A_MULTICORE, "CPU %zx running\n", cpu->getCpuID());
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
  if(ArchMulticore::CLSinitialized() && cpu_info.lapic.isInitialized())
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
  debug(A_MULTICORE, "AP switching from temp kernel pml4 to main kernel pml4: %zx\n", (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4));
  ArchMemory::loadPagingStructureRoot((size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4));

  debug(A_MULTICORE, "AP loading IDT, ptr at %p, base: %zx, limit: %zx\n", &InterruptUtils::idtr, (size_t)InterruptUtils::idtr.base, (size_t)InterruptUtils::idtr.limit);
  InterruptUtils::lidt(&InterruptUtils::idtr);

  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for AP [%p, %p)\n", &cls_start, &cls_end);
  ArchMulticore::setCLS(&cls_start, (size_t)&cls_end - (size_t)&cls_start);
  currentThread = NULL;

  ArchMulticore::initCLS();
  ArchThreads::initialise();

  debug(A_MULTICORE, "Enable AP timer\n");
  ArchInterrupts::enableTimer();

  char* cpu_stack_top = cpu_stack + sizeof(cpu_stack);
  debug(A_MULTICORE, "Switching to CPU local stack at %p\n", cpu_stack_top);
  __asm__ __volatile__("movq %[cpu_stack], %%rsp\n"
                       "movq %%rsp, %%rbp\n"
                       ::[cpu_stack]"m"(cpu_stack_top));
  waitForSystemStart();
}


void ArchMulticore::waitForSystemStart()
{
        kprintf("CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
        debug(A_MULTICORE, "CPU %zu initialized, waiting for system start\n", ArchMulticore::getCpuID());
        assert(CLSinitialized());
        ap_started = true;

        while(system_state != RUNNING);

        //debug(A_MULTICORE, "CPU %zu enabling interrupts\n", ArchMulticore::getCpuID());
        ArchInterrupts::enableInterrupts();

        while(1)
        {
                debug(A_MULTICORE, "AP %zu halting\n", ArchMulticore::getCpuID());
                __asm__ __volatile__("hlt\n");
        }
        assert(false);
}
