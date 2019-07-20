#include "types.h"
#include "PageManager.h"
#include "new.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include "assert.h"
#include "Bitmap.h"
#include "Allocator.h"


PageManager pm;

PageManager* PageManager::instance_ = 0;

PageManager* PageManager::instance()
{
  if (unlikely(!instance_))
    new (&pm) PageManager();
  return instance_;
}

extern void* kernel_start_address;
extern void* kernel_end_address;
extern uint8 boot_stack[0x4000];

enum MemType
{
        M_USEABLE = 1,
        M_ACPI_INFO = 3,
        M_RESERVED_PRESERVE_ON_HIBERNATION = 4,
        M_BAD = 5,
};

const char* multiboot_memtype[] = {
        "reserved",
        "available RAM",
        "reserved",
        "ACPI information",
        "reserved (preserve on hibernation)",
        "bad memory",
};

const char* getMultibootMemTypeString(size_t type)
{
        return (type < sizeof(multiboot_memtype)/sizeof(char*) ? multiboot_memtype[type] : "reserved");
}


PageManager::PageManager() :
        allocator_(nullptr),
        number_of_pages_(0),
        lock_("PageManager::lock_"),
        HEAP_PAGES(0)
{
  assert(instance_ == 0);
  instance_ = this;
  assert(KernelMemoryManager::instance_ == 0);

  BootstrapRangeAllocator bootstrap_pm{};
  allocator_ = &bootstrap_pm;

  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  size_t highest_address = 0;

  //Determine Amount of RAM
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    pointer start_address = 0, end_address = 0;
    size_t type = 0;
    ArchCommon::getUseableMemoryRegion(i, start_address, end_address, type);
    assert(type <= 5);
    debug(PM, "Ctor: memory region from physical %#zx to %#zx (%zu bytes) of type %zd [%s]\n",
          start_address, end_address, end_address - start_address, type, getMultibootMemTypeString(type));

    if (type == M_USEABLE)
    {
      highest_address = Max(highest_address, end_address);
      bootstrap_pm.setUseable(start_address, end_address);
    }
  }

  size_t total_num_useable_pages = bootstrap_pm.numUseablePages();
  number_of_pages_ = highest_address / PAGE_SIZE;

  bootstrap_pm.printUsageInfo();
  debug(PM, "Total useable pages: %zu (%zu bytes)\n", total_num_useable_pages, total_num_useable_pages*PAGE_SIZE);

  debug(PM, "Ctor: Marking pages used by the kernel as reserved\n");
  size_t kernel_virt_start = (size_t)&kernel_start_address;
  size_t kernel_virt_end   = (size_t)&kernel_end_address;
  size_t kernel_phys_start = kernel_virt_start - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
  size_t kernel_phys_end   = kernel_virt_end - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
  debug(PM, "Ctor: kernel start addr: %#zx, end addr: %#zx\n", kernel_virt_start, kernel_virt_end);
  debug(PM, "Ctor: kernel phys start addr: %#zx, phys end addr: %#zx\n", kernel_phys_start, kernel_phys_end);
  bootstrap_pm.setUnuseable(kernel_phys_start, kernel_phys_end);

  /*
  for (size_t i = ArchMemory::RESERVED_START; i < ArchMemory::RESERVED_END; ++i)
  {
    size_t physical_page = 0;
    size_t pte_page = 0;
    size_t this_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(i, &physical_page, &pte_page);
    assert(this_page_size == 0 || this_page_size == PAGE_SIZE || this_page_size == PAGE_SIZE * PAGE_TABLE_ENTRIES);
    if (this_page_size > 0)
    {
      //our bitmap only knows 4k pages for now
      uint64 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint64 p = 0; p < num_4kpages; ++p)
      {
        if (physical_page * num_4kpages + p < number_of_pages_)
          Bitmap::setBit(page_usage_table, used_pages, physical_page * num_4kpages + p);
      }
      i += (num_4kpages - 1); //+0 in most cases

      if (num_4kpages == 1 && i % 1024 == 0 && pte_page < number_of_pages_)
        Bitmap::setBit(page_usage_table, used_pages, pte_page);
    }
  }
  */

  debug(PM, "Ctor: Marking GRUB loaded modules as reserved\n");
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    debug(PM, "Ctor: module [%s]: start addr: %p, end addr: %p, phys start addr: %p, phys end addr: %p\n", ArchCommon::getModuleName(i), (void*)ArchCommon::getModuleStartAddress(i), (void*)ArchCommon::getModuleEndAddress(i), (void*)module_phys_start, (void*)module_phys_end);
    if(module_phys_end < module_phys_start)
    {
      continue;
    }
    size_t end_page = module_phys_end / PAGE_SIZE;

    bootstrap_pm.setUnuseable(module_phys_start, (end_page+1)*PAGE_SIZE);
  }

  debug(PM, "Ctor: Mapping GRUB loaded modules\n");
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    debug(PM, "Ctor: module [%s]: start addr: %p, end addr: %p, phys start addr: %p, phys end addr: %p\n", ArchCommon::getModuleName(i), (void*)ArchCommon::getModuleStartAddress(i), (void*)ArchCommon::getModuleEndAddress(i), (void*)module_phys_start, (void*)module_phys_end);
    if(module_phys_end < module_phys_start)
    {
      continue;
    }
    size_t start_page = module_phys_start / PAGE_SIZE;
    size_t end_page = module_phys_end / PAGE_SIZE;
    for (size_t k = Min(start_page, number_of_pages_); k <= Min(end_page, number_of_pages_ - 1); ++k)
    {
      if (ArchMemory::get_PPN_Of_VPN_In_KernelMapping(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, 0, 0) == 0)
      {
        if(PM & OUTPUT_ADVANCED)
                debug(PM, "Mapping kernel module at %#zx -> %#zx\n", (size_t)PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k);

        ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k, true);
      }
    }
  }
  debug(PM, "Finished mapping modules\n");

  debug(PM, "Before kernel heap allocation:\n");
  bootstrap_pm.printUsageInfo();
  debug(PM, "Num free pages: %zu\n", bootstrap_pm.numUseablePages());

  size_t num_pages_for_bitmap = (number_of_pages_ / 8) / PAGE_SIZE + 1;

  HEAP_PAGES = bootstrap_pm.numUseablePages()/3;
  if (HEAP_PAGES > 1024)
    HEAP_PAGES = 1024 + (HEAP_PAGES - Min(HEAP_PAGES, 1024))/8;


  debug(PM, "Num heap pages: %zu\n", HEAP_PAGES);


  size_t start_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE;
  // HACKY: Pages 0 + 1 are used for AP startup code
  size_t ap_boot_code_range = bootstrap_pm.alloc(PAGE_SIZE*2, PAGE_SIZE);
  assert(ap_boot_code_range == 0);

  size_t temp_page_size = 0;
  size_t num_reserved_heap_pages = 0;
  debug(PM, "Mapping reserved kernel heap pages\n");
  for (num_reserved_heap_pages = 0; num_reserved_heap_pages < num_pages_for_bitmap || temp_page_size != 0 ||
                                    num_reserved_heap_pages < ((DYNAMIC_KMM || (number_of_pages_ < 512)) ? 0 : HEAP_PAGES); ++num_reserved_heap_pages)
  {
    vpn_t vpn_to_map = start_vpn;

    ppn_t physical_page = 0;
    ppn_t pte_page = 0;
    if ((temp_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(vpn_to_map, &physical_page, &pte_page)) == 0)
    {
      ppn_t ppn_to_map = allocPPN();
      if(PM & OUTPUT_ADVANCED)
              debug(PM, "Mapping kernel heap vpn %p -> ppn %p\n", (void*)vpn_to_map, (void*)ppn_to_map);
      ArchMemory::mapKernelPage(vpn_to_map, ppn_to_map, true);
    }

    ++start_vpn;
  }

  extern KernelMemoryManager kmm;
  new (&kmm) KernelMemoryManager(num_reserved_heap_pages, HEAP_PAGES);

  debug(PM, "Allocating PM bitmap with %zx bits\n", number_of_pages_);
  allocator_ = new BitmapAllocator<PAGE_SIZE>(number_of_pages_*PAGE_SIZE);

  allocator_->setUnuseable(0, number_of_pages_*PAGE_SIZE);

  bootstrap_pm.printUsageInfo();
  debug(PM, "Num free pages: %zu\n", bootstrap_pm.numUseablePages());

  size_t free_phys_page;
  while((free_phys_page = bootstrap_pm.alloc(PAGE_SIZE, PAGE_SIZE)) != (size_t)-1)
  {
          allocator_->setUseable(free_phys_page, free_phys_page + PAGE_SIZE);
  }

  debug(PM, "Ctor: Physical pages - free: %zu used: %zu total: %zu\n", getNumFreePages(), total_num_useable_pages - getNumFreePages(), total_num_useable_pages);

  KernelMemoryManager::pm_ready_ = 1;
  debug(PM, "PM ctor finished\n");
}


uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}


size_t PageManager::getNumFreePages() const
{
  return allocator_->numFree()/PAGE_SIZE;
}


uint32 PageManager::allocPPN(uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  assert(allocator_);

  lock_.acquire();
  size_t phys_addr = allocator_->alloc(page_size, page_size);
  lock_.release();

  if (phys_addr == (size_t)-1)
  {
    assert(false && "PageManager::allocPPN: Out of memory / No more free physical pages");
  }

  ppn_t ppn = phys_addr/PAGE_SIZE;
  memset((void*)ArchMemory::getIdentAddressOfPPN(ppn), 0, page_size);
  return ppn;
}


void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  assert(allocator_);

  lock_.acquire();
  bool free_status = allocator_->dealloc(page_number*PAGE_SIZE, page_size);
  assert(free_status && "Double free PPN");
  lock_.release();
}




