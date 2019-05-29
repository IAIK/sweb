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


PageManager::PageManager() : lock_("PageManager::lock_")
{
  assert(instance_ == 0);
  instance_ = this;
  assert(KernelMemoryManager::instance_ == 0);
  number_of_pages_ = 0;
  lowest_unreserved_page_ = 0;

  BootstrapRangeAllocator bootstrap_pm{};

  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  size_t highest_address = 0;

  //Determine Amount of RAM
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    pointer start_address = 0, end_address = 0;
    size_t type = 0;
    ArchCommon::getUseableMemoryRegion(i, start_address, end_address, type);
    assert(type <= 5);
    debug(PM, "Ctor: memory region from physical %zx to %zx (%zu bytes) of type %zd [%s]\n",
          start_address, end_address, end_address - start_address, type, getMultibootMemTypeString(type));

    if (type == M_USEABLE)
    {
      highest_address = Max(highest_address, end_address);
      bootstrap_pm.markUseable(start_address, end_address);
    }
  }

  size_t total_num_useable_pages = bootstrap_pm.numUseablePages();
  number_of_pages_ = highest_address / PAGE_SIZE;

  bootstrap_pm.printUseableRanges();
  debug(PM, "Total useable pages: %zx\n", total_num_useable_pages);

  debug(PM, "Ctor: Marking pages used by the kernel as reserved\n");
  size_t kernel_virt_start = (size_t)&kernel_start_address;
  size_t kernel_virt_end   = (size_t)&kernel_end_address;
  size_t kernel_phys_start = kernel_virt_start - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
  size_t kernel_phys_end   = kernel_virt_end - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
  debug(PM, "Ctor: kernel start addr: %zx, end addr: %zx\n", kernel_virt_start, kernel_virt_end);
  debug(PM, "Ctor: kernel phys start addr: %zx, phys end addr: %zx\n", kernel_phys_start, kernel_phys_end);
  bootstrap_pm.markUnuseable(kernel_phys_start, kernel_phys_end);

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
    debug(PM, "Ctor: module [%s]: start addr: %zx, end addr: %zx\n", ArchCommon::getModuleName(i), ArchCommon::getModuleStartAddress(i), ArchCommon::getModuleEndAddress(i));
    size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
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
        ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k,k);

      }
    }
    bootstrap_pm.markUnuseable(module_phys_start, (end_page+1)*PAGE_SIZE);
  }
  debug(PM, "Finished mapping modules\n");

  debug(PM, "Before kernel heap allocation:\n");
  bootstrap_pm.printUseableRanges();
  debug(PM, "Num free pages: %zx\n", bootstrap_pm.numUseablePages());

  size_t num_pages_for_bitmap = (number_of_pages_ / 8) / PAGE_SIZE + 1;

  HEAP_PAGES = bootstrap_pm.numUseablePages()/3;
  if (HEAP_PAGES > 1024)
    HEAP_PAGES = 1024 + (HEAP_PAGES - Min(HEAP_PAGES, 1024))/8;


  debug(PM, "Num heap pages: %zx\n", HEAP_PAGES);


  size_t start_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE;
  // HACKY: Pages 0 + 1 are used for AP startup code
  size_t ap_boot_code_range = bootstrap_pm.allocRange(PAGE_SIZE*2, PAGE_SIZE);
  assert(ap_boot_code_range == 0);

  size_t temp_page_size = 0;
  size_t num_reserved_heap_pages = 0;
  debug(PM, "Mapping reserved heap pages\n");
  for (num_reserved_heap_pages = 0; num_reserved_heap_pages < num_pages_for_bitmap || temp_page_size != 0 ||
                                    num_reserved_heap_pages < ((DYNAMIC_KMM || (number_of_pages_ < 512)) ? 0 : HEAP_PAGES); ++num_reserved_heap_pages)
  {
    ppn_t ppn_to_map = bootstrap_pm.allocRange(PAGE_SIZE, PAGE_SIZE);
    if(ppn_to_map == (size_t)-1) break;
    ppn_to_map /= PAGE_SIZE;
    vpn_t vpn_to_map = start_vpn;

    ppn_t physical_page = 0;
    ppn_t pte_page = 0;
    if ((temp_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(vpn_to_map, &physical_page, &pte_page)) == 0)
    {
      // TODO: Not architecture independent
      ArchMemoryMapping m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()) / PAGE_SIZE), vpn_to_map);
      if(m.pt_ppn != 0)
      {
              ArchMemory::mapKernelPage(vpn_to_map, ppn_to_map);
      }
      else
      {
              debug(PM, "No PT present at %zx, abort heap mapping\n", vpn_to_map);
              break;
      }
    }
    ++start_vpn;
  }
  debug(PM, "Finished mapping %zx reserved heap pages\n", num_reserved_heap_pages);

  extern KernelMemoryManager kmm;
  new (&kmm) KernelMemoryManager(num_reserved_heap_pages, HEAP_PAGES);
  debug(PM, "Allocating PM bitmap with %zx bits\n", number_of_pages_);
  page_usage_table_ = new Bitmap(number_of_pages_);

  for(size_t i = 0; i < number_of_pages_; ++i)
  {
          page_usage_table_->setBit(i);
  }

  bootstrap_pm.printUseableRanges();
  debug(PM, "Num free pages: %zx\n", bootstrap_pm.numUseablePages());

  size_t free_phys_page = bootstrap_pm.allocRange(PAGE_SIZE, PAGE_SIZE);
  while(free_phys_page != (size_t)-1)
  {
          page_usage_table_->unsetBit(free_phys_page/PAGE_SIZE);
          free_phys_page = bootstrap_pm.allocRange(PAGE_SIZE, PAGE_SIZE);
  }

  for (size_t p = 0; p < number_of_pages_; ++p)
  {
    if (!page_usage_table_->getBit(p))
    {
      lowest_unreserved_page_ = p;
      break;
    }
  }

  debug(PM, "Ctor: Physical pages - free: %zu used: %zu total: %zu\n", page_usage_table_->getNumFreeBits(), total_num_useable_pages - page_usage_table_->getNumFreeBits(), total_num_useable_pages);

  assert(lowest_unreserved_page_ < number_of_pages_);
  KernelMemoryManager::pm_ready_ = 1;
  debug(PM, "PM ctor finished\n");
}

uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}

size_t PageManager::getNumFreePages() const
{
  return page_usage_table_->getNumFreeBits();
}

bool PageManager::reservePages(uint32 ppn, uint32 num)
{
  assert(lock_.heldBy() == currentThread);
  if (ppn < number_of_pages_ && !page_usage_table_->getBit(ppn))
  {
    if (num == 1 || reservePages(ppn + 1, num - 1))
    {
      page_usage_table_->setBit(ppn);
      return true;
    }
  }
  return false;
}

uint32 PageManager::allocPPN(uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  lock_.acquire();
  uint32 p;
  uint32 found = 0;
  for (p = lowest_unreserved_page_; !found && p < number_of_pages_; ++p)
  {
    if ((p % (page_size / PAGE_SIZE)) != 0)
      continue;
    if (reservePages(p, page_size / PAGE_SIZE))
      found = p;
  }

  while (lowest_unreserved_page_ < number_of_pages_ && page_usage_table_->getBit(lowest_unreserved_page_))
  {
    ++lowest_unreserved_page_;
  }
  lock_.release();

  if (found == 0)
  {
    assert(false && "PageManager::allocPPN: Out of memory / No more free physical pages");
  }
  memset((void*)ArchMemory::getIdentAddressOfPPN(found), 0, page_size);
  return found;
}

void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  lock_.acquire();
  if (page_number < lowest_unreserved_page_)
    lowest_unreserved_page_ = page_number;
  for (uint32 p = page_number; p < (page_number + page_size / PAGE_SIZE); ++p)
  {
    assert(page_usage_table_->getBit(p) && "Double free PPN");
    page_usage_table_->unsetBit(p);
  }
  lock_.release();
}





/*
  Does not merge overlapping ranges when ranges are expanded!
 */
void BootstrapRangeAllocator::markUseable(__attribute__((unused)) size_t start, __attribute__((unused)) size_t end)
{
        debug(PM, "markUseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if((start >= useable_ranges_[i].start) &&
                   (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "markUseable [%zx - %zx): already covered by [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        return;
                }
                else if((start <= useable_ranges_[i].start) &&
                        (end >= useable_ranges_[i].end))
                {
                        //debug(PM, "markUseable [%zx - %zx) completely covers [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = start;
                        useable_ranges_[i].end = end;
                        return;
                }
                else if((start >= useable_ranges_[i].start) &&
                        (start <= useable_ranges_[i].end))
                {
                        //debug(PM, "markUseable [%zx - %zx) expands end [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].end = Max(useable_ranges_[i].end, end);
                        return;
                }
                else if((end >= useable_ranges_[i].start) &&
                        (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "markUseable [%zx - %zx) expands start [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = Min(useable_ranges_[i].start, start);
                        return;
                }
        }

        ssize_t slot = findFirstFreeSlot();
        assert(slot != -1);

        useable_ranges_[slot].start = start;
        useable_ranges_[slot].end = end;
}

void BootstrapRangeAllocator::markUnuseable(__attribute__((unused)) size_t start, __attribute__((unused)) size_t end)
{
        debug(PM, "markUnuseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if((start > useable_ranges_[i].start) &&
                        (end < useable_ranges_[i].end))
                {
                        //debug(PM, "markUnuseable [%zx - %zx) splits [%zx - %zx) into [%zx - %zx)+[%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end, useable_ranges_[i].start, start, end, useable_ranges_[i].end);
                        size_t prev_end = useable_ranges_[i].end;
                        useable_ranges_[i].end = start;
                        markUseable(end, prev_end);
                }
                else if((end > useable_ranges_[i].start) &&
                        (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "markUnuseable [%zx - %zx) moves start of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = Min(end, useable_ranges_[i].end);
                }
                else if((start >= useable_ranges_[i].start) &&
                        (start < useable_ranges_[i].end))
                {
                        //debug(PM, "markUnuseable [%zx - %zx) moves end of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].end = Min(start, useable_ranges_[i].end);
                }

        }
}

ssize_t BootstrapRangeAllocator::findFirstFreeSlot()
{
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(!slotIsUsed(i))
                {
                        return i;
                }
        }

        return -1;
}

bool BootstrapRangeAllocator::slotIsUsed(size_t i)
{
        assert(i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]));
        return useable_ranges_[i].start != useable_ranges_[i].end;
}

void BootstrapRangeAllocator::printUseableRanges()
{
        debug(PM, "Bootstrap PM useable ranges:\n");

        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(slotIsUsed(i))
                {
                        debug(PM, "[%zx - %zx)\n", useable_ranges_[i].start, useable_ranges_[i].end);
                }
        }
}

size_t BootstrapRangeAllocator::numUseablePages()
{
        size_t num_useable_pages = 0;
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(slotIsUsed(i))
                {
                        num_useable_pages += (useable_ranges_[i].end - useable_ranges_[i].start)/PAGE_SIZE;
                }
        }
        return num_useable_pages;
}

size_t BootstrapRangeAllocator::allocRange(size_t size, size_t alignment)
{
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                size_t start = useable_ranges_[i].start;
                if(start % alignment != 0)
                {
                        start = start - (start % alignment) + alignment;
                }
                if(start + size <= useable_ranges_[i].end)
                {
                        //debug(PM, "Bootstrap PM allocating range [%zx-%zx)\n", start, start+size);
                        useable_ranges_[i].start = start + size;
                        return start;
                }
        }

        return -1;
}
