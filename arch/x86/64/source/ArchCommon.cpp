#include "ArchCommon.h"

#include "ACPI.h"
#include "FrameBufferConsole.h"
#include "IDEDriver.h"
#include "KernelMemoryManager.h"
#include "PageManager.h"
#include "PlatformBus.h"
#include "ProgrammableIntervalTimer.h"
#include "SMP.h"
#include "SWEBDebugInfo.h"
#include "Scheduler.h"
#include "SegmentUtils.h"
#include "SerialManager.h"
#include "TextConsole.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include "kstring.h"
#include "multiboot.h"
#include "offsets.h"
#include "ports.h"

#include "ArchMemory.h"
#include "ArchMulticore.h"

void puts(const char* string);

#if (A_BOOT == A_BOOT | OUTPUT_ENABLED)
#define PRINT(X) writeLine2Bochs((const char*)VIRTUAL_TO_PHYSICAL_BOOT(X))
#else
#define PRINT(X)
#endif

RangeAllocator<> mmio_addr_allocator;

extern void* kernel_start_address;
extern void* kernel_end_address;

__attribute__ ((section (".data"))) multiboot_info_t* multi_boot_structure_pointer = (multiboot_info_t*)0xDEADDEAD; // must not be in bss segment
__attribute__ ((section (".data"))) static struct multiboot_remainder mbr; // must not be in bss segment

extern "C" void parseMultibootHeader()
{
  uint32 i;
  multiboot_info_t *mb_infos = *(multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT( (pointer)&multi_boot_structure_pointer);
  struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));

  PRINT("Bootloader: ");
  writeLine2Bochs((char*)(pointer)(mb_infos->boot_loader_name));
  PRINT("\n");

  if (mb_infos && mb_infos->f_cmdline)
  {
    const char* cmdline = (char*)(uintptr_t)mb_infos->cmdline;
    size_t len = strlen(cmdline);
    if (len+1 <= sizeof(orig_mbr.cmdline))
    {
        memcpy(orig_mbr.cmdline, cmdline, len+1);
    }
  }

  if (mb_infos && mb_infos->f_fb)
  {
    orig_mbr.have_framebuffer = true;
    orig_mbr.framebuffer = mb_infos->framebuffer;
  }

  if (mb_infos && mb_infos->f_vbe)
  {
    orig_mbr.have_vbe = true;
    orig_mbr.vbe = mb_infos->vbe;
    struct vbe_mode* mode_info = (struct vbe_mode*)(uint64)mb_infos->vbe.vbe_mode_info;
    orig_mbr.have_vesa_console = 1;
    orig_mbr.vesa_lfb_pointer = mode_info->phys_base;
    orig_mbr.vesa_x_res = mode_info->x_resolution;
    orig_mbr.vesa_y_res = mode_info->y_resolution;
    orig_mbr.vesa_bits_per_pixel = mode_info->bits_per_pixel;
  }

  if (mb_infos && mb_infos->f_mods)
  {
    module_t * mods = (module_t*)(uint64)mb_infos->mods_addr;
    for (i=0;i<mb_infos->mods_count;++i)
    {
      orig_mbr.module_maps[i].used = 1;
      orig_mbr.module_maps[i].start_address = mods[i].mod_start;
      orig_mbr.module_maps[i].end_address = mods[i].mod_end;
      strncpy((char*)(uint64)orig_mbr.module_maps[i].name, (const char*)(uint64)mods[i].string, 256);
    }
    orig_mbr.num_module_maps = mb_infos->mods_count;
  }

  for (i=0;i<MAX_MEMORY_MAPS;++i)
  {
    orig_mbr.memory_maps[i].used = 0;
  }

  if (mb_infos && mb_infos->f_mmap)
  {
    size_t i = 0;
    memory_map * map = (memory_map*)(uint64)(mb_infos->mmap_addr);
    while((uint64)map < (uint64)(mb_infos->mmap_addr + mb_infos->mmap_length))
    {
      orig_mbr.memory_maps[i].used          = 1;
      orig_mbr.memory_maps[i].start_address = ((uint64)map->base_addr_high << 32) | ((uint64)map->base_addr_low);
      orig_mbr.memory_maps[i].end_address   = orig_mbr.memory_maps[i].start_address
                                              + (((uint64)map->length_high << 32) | ((uint64)map->length_low));
      orig_mbr.memory_maps[i].type          = map->type;

      map = (memory_map*)(((uint64)(map)) + map->size + sizeof(map->size));
      ++i;
    }
  }

  if (mb_infos && mb_infos->f_elf_shdr)
  {
    orig_mbr.have_elf_sec_hdr = true;
    orig_mbr.elf_sec = mb_infos->elf_sec;
  }
}

pointer ArchCommon::getKernelStartAddress()
{
   return (pointer)&kernel_start_address;
}

pointer ArchCommon::getKernelEndAddress()
{
   return (pointer)&kernel_end_address;
}

pointer ArchCommon::getFreeKernelMemoryStart()
{
  pointer free_kernel_memory_start = (pointer)&kernel_end_address;
  for (size_t i = 0; i < getNumModules(); ++i)
    free_kernel_memory_start = Max(getModuleEndAddress(i),free_kernel_memory_start);
  return ((free_kernel_memory_start - 1) | 0xFFF) + 1;
}

pointer ArchCommon::getFreeKernelMemoryEnd()
{
    if (KernelMemoryManager::isReady())
    {
        return KernelMemoryManager::instance()->getKernelBreak();
    }
    else
    {
        return ArchCommon::getKernelEndAddress();
    }
}


size_t ArchCommon::haveVESAConsole(size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.have_vesa_console;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.have_vesa_console;
  }
}

size_t ArchCommon::getNumModules(size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.num_module_maps;
  else
  {
    struct multiboot_remainder* orig_mbr = &(struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr->num_module_maps;
  }
}

const char* ArchCommon::getModuleName(size_t num, size_t is_paging_set_up)
{
        if (is_paging_set_up)
                return (char*)((size_t)mbr.module_maps[num].name | PHYSICAL_TO_VIRTUAL_OFFSET);
        else
        {
                struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
                return (char*)orig_mbr.module_maps[num].name;
        }
}

size_t ArchCommon::getModuleStartAddress(size_t num, size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].start_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].start_address;
  }
}

size_t ArchCommon::getModuleEndAddress(size_t num, size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].end_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].end_address;
  }
}

size_t ArchCommon::getVESAConsoleHeight()
{
  return mbr.vesa_y_res;
}

size_t ArchCommon::getVESAConsoleWidth()
{
  return mbr.vesa_x_res;
}

pointer ArchCommon::getVESAConsoleLFBPtr(size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return 0xFFFFFFFFC0000000ULL - 1024ULL * 1024ULL * 16ULL;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.vesa_lfb_pointer;
  }
}

pointer ArchCommon::getFBPtr(size_t is_paging_set_up)
{
  if (is_paging_set_up)
    return PHYSICAL_TO_VIRTUAL_OFFSET | 0xB8000ULL;
  else
    return 0xB8000ULL;
}

size_t ArchCommon::getFBWidth()
{
    return 80;
}

size_t ArchCommon::getFBHeight()
{
    return 25;
}

size_t ArchCommon::getFBBitsPerCharacter()
{
    return 16;
}

size_t ArchCommon::getFBSize()
{
    return getFBWidth() * getFBHeight() * getFBBitsPerCharacter()/8;
}

size_t ArchCommon::getVESAConsoleBitsPerPixel()
{
  return mbr.vesa_bits_per_pixel;
}

size_t ArchCommon::getNumUseableMemoryRegions()
{
  uint32 i;
  for (i=0;i<MAX_MEMORY_MAPS;++i)
  {
    if (!mbr.memory_maps[i].used)
      break;
  }
  return i;
}

size_t ArchCommon::getUseableMemoryRegion(size_t region, pointer &start_address, pointer &end_address, size_t &type)
{
  if (region >= MAX_MEMORY_MAPS)
    return 1;

  start_address = mbr.memory_maps[region].start_address;
  end_address = mbr.memory_maps[region].end_address;
  type = mbr.memory_maps[region].type;

  return 0;
}

Console* ArchCommon::createConsole(size_t count)
{
  // deactivate cursor
  outportb(0x3d4, 0xa);
  outportb(0x3d5, 0b00100000);
  if (haveVESAConsole())
    return new FrameBufferConsole(count);
  else
    return new TextConsole(count);
}


extern GDT gdt;
extern "C" void startup();
extern "C" void initialisePaging();
extern uint8 boot_stack[0x4000];

GDT64Ptr gdt_ptr;

extern "C" [[noreturn]] void entry64()
{
  PRINT("Parsing Multiboot Header...\n");
  parseMultibootHeader();
  PRINT("Initializing Kernel Paging Structures...\n");
  initialisePaging();
  PRINT("Setting CR3 Register...\n");
  asm("mov %%rax, %%cr3" : : "a"(VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getKernelPagingStructureRootVirt())));
  kprintf("Paging initialized\n");
  PRINT("Switch to our own stack...\n");
  kprintf("Switch to our own stack...\n");
  asm("mov %[stack], %%rsp\n"
      "mov %[stack], %%rbp\n" : : [stack]"i"(boot_stack + 0x4000));
  PRINT("Loading Long Mode Segments...\n");
  kprintf("Loading Long Mode Segments...\n");

  if (A_BOOT & OUTPUT_ADVANCED)
  {
    kprintf("GDT: %p\n", &gdt);
    kprintf("GDT[0]: %lx\n", *(uint64*)&gdt.entries[0]);
    kprintf("GDT[0]: %lx\n", *(uint64*)&gdt.entries[1]);
    kprintf("GDT[2]: %lx\n", *(uint64*)&gdt.entries[2]);
    kprintf("GDT[3]: %lx\n", *(uint64*)&gdt.entries[3]);
    kprintf("GDT: %p\n", VIRTUAL_TO_PHYSICAL_BOOT(&gdt));
    kprintf("GDT[0]: %lx\n", *(uint64*)&(((GDT*)VIRTUAL_TO_PHYSICAL_BOOT(&gdt))->entries[0]));
    kprintf("GDT[0]: %lx\n", *(uint64*)&(((GDT*)VIRTUAL_TO_PHYSICAL_BOOT(&gdt))->entries[1]));
    kprintf("GDT[2]: %lx\n", *(uint64*)&(((GDT*)VIRTUAL_TO_PHYSICAL_BOOT(&gdt))->entries[2]));
    kprintf("GDT[3]: %lx\n", *(uint64*)&(((GDT*)VIRTUAL_TO_PHYSICAL_BOOT(&gdt))->entries[3]));
    kprintf("PML4[0]: %lx\n", *(uint64*)&kernel_page_map_level_4[0]);
    kprintf("PDPT[0]: %lx\n", *(uint64*)&kernel_page_directory_pointer_table[0].pd);
    kprintf("PD[0]: %lx\n", *(uint64*)&kernel_page_directory[0].page);
    kprintf("PD[1]: %lx\n", *(uint64*)&kernel_page_directory[1].page);
    kprintf("PD[2]: %lx\n", *(uint64*)&kernel_page_directory[2].page);
  }

  assert(kernel_page_directory[0].page.present);
  assert(kernel_page_directory[0].page.size);
  assert(kernel_page_directory[0].page.page_ppn == 0);
  assert(kernel_page_directory[1].page.present);
  assert(kernel_page_directory[1].page.size);
  assert(kernel_page_directory[1].page.page_ppn == 1);
  assert(*(uint64*)&gdt.entries[0] == 0);
  assert(*(uint64*)&gdt.entries[1] != 0);
  assert(*(uint64*)&gdt.entries[2] != 0);

  gdt_ptr.limit = sizeof(gdt) - 1;
  gdt_ptr.addr = (uint64)&gdt;
  asm("lgdt (%%rax)" : : "a"(&gdt_ptr));
  asm("mov %%ax, %%ds\n"
      "mov %%ax, %%es\n"
      "mov %%ax, %%ss\n"
      "mov %%ax, %%fs\n"
      "mov %%ax, %%gs\n"
      : : "a"(KERNEL_DS));
  PRINT("Reloading TSS...\n");
  kprintf("Reloading TSS...\n");
  asm("ltr %%ax" : : "a"(KERNEL_TSS));


  extern char cls_start;
  extern char cls_end;
  debug(A_MULTICORE, "Setting temporary CLS for boot processor [%p, %p)\n", &cls_start, &cls_end);
  CpuLocalStorage::setCls(&cls_start);
  currentThread = nullptr;

  PRINT("Calling startup()...\n");
  kprintf("Calling startup()...\n");
  asm("jmp *%[startup]" : : [startup]"r"(startup));
  assert(false && "Returned from startup");
  while (1);
}

class Stabs2DebugInfo;
const Stabs2DebugInfo* kernel_debug_info = 0;

void ArchCommon::initDebug()
{
  debug(A_COMMON, "initDebug\n");
  for (size_t i = 0; i < getNumModules(); ++i)
  {
    debug(A_COMMON, "Checking module from [%zx -> %zx)\n", getModuleStartAddress(i), getModuleEndAddress(i));
    if ((getModuleStartAddress(i) < getModuleEndAddress(i)) &&
        (memcmp("SWEBDBG1", (const char*)getModuleStartAddress(i), 8) == 0))
    {
        kernel_debug_info = new SWEBDebugInfo((const char*)getModuleStartAddress(i),
                                              (const char*)getModuleEndAddress(i));
    }
  }
  if (!kernel_debug_info)
  {
    kernel_debug_info = new SWEBDebugInfo(0, 0);
  }
  debug(A_COMMON, "initDebug done\n");
}

void ArchCommon::idle()
{
  halt();
}

void ArchCommon::halt()
{
  asm volatile("hlt");
}

#define STATS_OFFSET 22

void ArchCommon::drawStat() {
    const char* text  = "Free pages      F9 MemInfo   F10 Locks   F11 Stacktrace   F12 Threads";
    const char* color = "xxxxxxxxxx      xx           xxx         xxx              xxx        ";

    char* fb = (char*)getFBPtr();
    size_t i = 0;
    while(text[i]) {
        fb[i * 2 + STATS_OFFSET] = text[i];
        fb[i * 2 + STATS_OFFSET + 1] = (char)(color[i] == 'x' ? ((CONSOLECOLOR::BLACK) | (CONSOLECOLOR::DARK_GREY << 4)) :
                                                                ((CONSOLECOLOR::DARK_GREY) | (CONSOLECOLOR::BLACK << 4)));
        i++;
    }

    char itoa_buffer[80];

#define STATS_FREE_PAGES_START (STATS_OFFSET + 11*2)
    memset(fb + STATS_FREE_PAGES_START, 0, 4*2);
    memset(itoa_buffer, '\0', sizeof(itoa_buffer));
    itoa(PageManager::instance().getNumFreePages(), itoa_buffer, 10);
    for(size_t i = 0; (i < sizeof(itoa_buffer)) && (itoa_buffer[i] != '\0'); ++i)
    {
      fb[STATS_FREE_PAGES_START + i * 2] = itoa_buffer[i];
      fb[STATS_FREE_PAGES_START + i * 2 + 1] = ((CONSOLECOLOR::WHITE) | (CONSOLECOLOR::BLACK << 4));
    }

#define STATS_FREE_PAGES_PERCENT_START (STATS_OFFSET + 80*2 + 11*2)
    size_t total_pages = PageManager::instance().getTotalNumPages();
    size_t free_pages_percent = total_pages ? (PageManager::instance().getNumFreePages()*100)/total_pages : 0;
    memset(fb + STATS_FREE_PAGES_PERCENT_START, 0, 4*2);
    memset(itoa_buffer, '\0', sizeof(itoa_buffer));
    itoa(free_pages_percent, itoa_buffer, 10);
    size_t free_pp_len = strlen(itoa_buffer);
    itoa_buffer[free_pp_len] = '%';
    for(size_t i = 0; (i < sizeof(itoa_buffer)) && (itoa_buffer[i] != '\0'); ++i)
    {
        fb[STATS_FREE_PAGES_PERCENT_START + i * 2] = itoa_buffer[i];
        fb[STATS_FREE_PAGES_PERCENT_START + i * 2 + 1] = ((CONSOLECOLOR::WHITE) | (CONSOLECOLOR::BLACK << 4));
    }

#define STATS_NUM_THREADS_START (80*2 + 73*2)
    memset(fb + STATS_NUM_THREADS_START, 0, 4*2);
    memset(itoa_buffer, '\0', sizeof(itoa_buffer));
    itoa(Scheduler::instance()->num_threads, itoa_buffer, 10);
    for(size_t i = 0; (i < sizeof(itoa_buffer)) && (itoa_buffer[i] != '\0'); ++i)
    {
            fb[STATS_NUM_THREADS_START + i * 2] = itoa_buffer[i];
            fb[STATS_NUM_THREADS_START + i * 2 + 1] = ((CONSOLECOLOR::WHITE) | (CONSOLECOLOR::BLACK << 4));
    }


    size_t STATS_SCHED_LOCK_CONTENTION_START = (80*2 + SMP::numRunningCpus()*2);
    // calc fixnum xxx.xxx%
    size_t sched_lock_free = Scheduler::instance()->scheduler_lock_count_free;
    size_t sched_lock_blocked = Scheduler::instance()->scheduler_lock_count_blocked;
    size_t sched_lock_total = sched_lock_free + sched_lock_blocked;
    size_t sched_lock_contention_percent = sched_lock_total ? (sched_lock_blocked*100)/sched_lock_total : 0;
    size_t sched_lock_contention_2 = sched_lock_total ? ((sched_lock_blocked*100000)/sched_lock_total) % 1000 : 0;

    memset(itoa_buffer, '\0', sizeof(itoa_buffer));
    itoa(sched_lock_contention_percent, itoa_buffer, 10);
    size_t slc_len = strlen(itoa_buffer);
    itoa_buffer[slc_len++] = '.';
    if (sched_lock_contention_2 < 100)
        itoa_buffer[slc_len++] = '0';
    if (sched_lock_contention_2 < 10)
        itoa_buffer[slc_len++] = '0';
    itoa(sched_lock_contention_2, itoa_buffer + slc_len, 10);
    slc_len = strlen(itoa_buffer);
    itoa_buffer[slc_len] = '%';

    memset(fb + STATS_SCHED_LOCK_CONTENTION_START, 0, 7*2);
    for(size_t i = 0; (i < sizeof(itoa_buffer)) && (itoa_buffer[i] != '\0'); ++i)
    {
        fb[STATS_SCHED_LOCK_CONTENTION_START + i * 2] = itoa_buffer[i];
        fb[STATS_SCHED_LOCK_CONTENTION_START + i * 2 + 1] = ((CONSOLECOLOR::WHITE) | (CONSOLECOLOR::BLACK << 4));
    }
}

void updateStatsThreadColor()
{
    char* fb = (char*)ArchCommon::getFBPtr();
    fb[1 + SMP::currentCpuId()*2] =
        (((currentThread ? currentThread->console_color :
           CONSOLECOLOR::BLACK) << 4) |
         CONSOLECOLOR::BRIGHT_WHITE);
}

cpu_local size_t heart_beat_value = 0;

void ArchCommon::drawHeartBeat()
{
  drawStat();

  const char* clock = "/-\\|";
  char* fb = (char*)getFBPtr();
  size_t cpu_id = SMP::currentCpuId();
  fb[0 + cpu_id*2] = clock[heart_beat_value++ % 4];
  updateStatsThreadColor();
}


void ArchCommon::postBootInit()
{
  initACPI();
}

void ArchCommon::initPlatformDrivers()
{
    PlatformBus::instance().registerDriver(PITDriver::instance());
    PlatformBus::instance().registerDriver(SerialManager::instance());
}

void ArchCommon::initBlockDeviceDrivers()
{
    PlatformBus::instance().registerDriver(IDEControllerDriver::instance());
}

[[noreturn]] void ArchCommon::callWithStack(char* stack, void (*func)())
{
  asm volatile("movq %[stack], %%rsp\n"
               "callq *%[func]\n"
               ::[stack]"r"(stack),
                 [func]"r"(func));
  assert(false);
}


uint64 ArchCommon::cpuTimestamp()
{
    uint64 low, high;
    asm volatile("rdtsc\n"
                 :"=a"(low), "=d"(high));
    return (high << 32) | low;
}

void ArchCommon::spinlockPause()
{
    asm volatile("pause\n");
}

void ArchCommon::reservePagesPreKernelInit(Allocator &alloc)
{
    ArchMulticore::reservePages(alloc);
}

void ArchCommon::initKernelVirtualAddressAllocator()
{
    mmio_addr_allocator.setUseable(KERNEL_START, (size_t)-1);
    mmio_addr_allocator.setUnuseable(getKernelStartAddress(), getKernelEndAddress());
    mmio_addr_allocator.setUnuseable(KernelMemoryManager::instance()->getKernelHeapStart(), KernelMemoryManager::instance()->getKernelHeapMaxEnd());
    mmio_addr_allocator.setUnuseable(IDENT_MAPPING_START, IDENT_MAPPING_END);
    debug(MAIN, "Usable MMIO ranges:\n");
    mmio_addr_allocator.printUsageInfo();
}
