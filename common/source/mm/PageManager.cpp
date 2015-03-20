#include "PageManager.h"
#include "new.h"
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

// Between min and max KMM heap memory is reserved dynamically!
// Please note that this means that the KMM depends on the page manager and you
// will have a harder time implementing swapping. Pros only!
// Please also note that the implementation of *dynamic* KMM is rather new - please report any strange behavior.

// we recommend setting both values to 400 for Assignment 2:
#define MIN_HEAP_PAGES 1
#define MAX_HEAP_PAGES 4096 // 4096 pages, because maximum heap size is 16MiB

PageManager* PageManager::instance()
{
  if (unlikely(!instance_))
    new (&pm) PageManager();
  return instance_;
}

PageManager::PageManager() : lock_("PageManager::lock_")
{
  assert(instance_ == 0);
  instance_ = this;
  assert(KernelMemoryManager::instance_ == 0);
  number_of_pages_ = 0;
  lowest_unreserved_page_ = 0;

  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  pointer start_address = 0, end_address = 0, last_end_page = lowest_unreserved_page_;
  size_t highest_address = 0, type = 0, used_pages = 0;

  //Determine Amount of RAM
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    ArchCommon::getUsableMemoryRegion(i, start_address, end_address, type);
    debug(PM, "Ctor: usable memory region from physical %x to %x of type %d\n", start_address, end_address, type);
    if (type == 1)
      highest_address = Max(highest_address, end_address & 0x7FFFFFFF);
  }

  number_of_pages_ = highest_address / PAGE_SIZE;

  //Determine RAM Used by Grub Modules
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page = (ArchCommon::getModuleStartAddress(i) & 0x3FFFFFFF) / PAGE_SIZE;
    uint32 end_page = (ArchCommon::getModuleEndAddress(i) & 0x3FFFFFFF) / PAGE_SIZE;

    assert(start_page <= last_end_page);
    if (start_page > end_page)
      break;
    used_pages += end_page - start_page + ((i > 0 && end_page == last_end_page) ? 0 : 1);
    last_end_page = end_page;
  }
  lowest_unreserved_page_ = last_end_page;

  //need at least 4 MiB for Kernel Memory + first physical MiB
  if (number_of_pages_ < 1000)
  {
    kprintfd("FATAL ERROR: Not enough Memory, Sweb needs at least %d KiB of RAM\n", 1000 * 4096U);
    prenew_assert(false);
  }

  size_t num_pages_for_bitmap = (number_of_pages_ / 8) / PAGE_SIZE + 1;
  size_t start_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE;
  size_t last_free_page = number_of_pages_-1;
  size_t temp_page_size = 0;
  size_t num_reserved_heap_pages = 0;
  for (num_reserved_heap_pages = 0; num_reserved_heap_pages < num_pages_for_bitmap || temp_page_size != 0 ||
                                    num_reserved_heap_pages < MIN_HEAP_PAGES; ++num_reserved_heap_pages)
  {
    if ((temp_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(start_vpn,0,0)) == 0)
      ArchMemory::mapKernelPage(start_vpn,last_free_page--);
    kprintfd("map %x -> %x\n", start_vpn, last_free_page-1);
    start_vpn++;
  }
  extern KernelMemoryManager kmm;
  new (&kmm) KernelMemoryManager(num_reserved_heap_pages,MAX_HEAP_PAGES);
  page_usage_table_ = new Bitmap(number_of_pages_);

  // since we have gaps in the memory maps we can not give out everything
  // first mark everything as reserved, just to be sure
  debug(PM, "Ctor: Initializing page_usage_table_ with all pages reserved\n");
  for (size_t i = 0; i < number_of_pages_; ++i)
  {
    page_usage_table_->setBit(i);
  }

  //now mark as free, everything that might be useable
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    ArchCommon::getUsableMemoryRegion(i, start_address, end_address, type);
    if (type != 1)
      continue;
    uint32 start_page = start_address / PAGE_SIZE;
    uint32 end_page = end_address / PAGE_SIZE;
    debug(PM, "Ctor: usable memory region: start_page: %d, end_page: %d, type: %d\n", start_page, end_page, type);

    for (size_t k = Max(start_page, lowest_unreserved_page_); k < Min(end_page, number_of_pages_); ++k)
    {
      page_usage_table_->unsetBit(k);
    }
  }

  //some of the usable memory regions are already in use by the kernel (within first 1024 pages)
  //therefore, mark as reserved everything >2gb und <3gb already used in PageDirectory
  debug(PM, "Ctor: Marking stuff mapped in above 2 and < 3 gig as used\n");
//  uint32 last_page=0;
  for (size_t i = ArchMemory::RESERVED_START; i < ArchMemory::RESERVED_END; ++i)
  {
    size_t physical_page = 0;
    size_t pte_page = 0;
    size_t this_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(i, &physical_page, &pte_page);
    assert(this_page_size == 0 || this_page_size == PAGE_SIZE || this_page_size == PAGE_SIZE*PAGE_TABLE_ENTRIES);
    if (this_page_size > 0)
    {
      //our bitmap only knows 4k pages for now
      uint64 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint64 p = 0; p < num_4kpages; ++p)
        if (physical_page * num_4kpages + p < number_of_pages_)
          page_usage_table_->setBit(physical_page * num_4kpages + p);
      i += (num_4kpages - 1); //+0 in most cases
      if (num_4kpages == 1 && i % 1024 == 0 && pte_page < number_of_pages_)
        page_usage_table_->setBit(pte_page);
    }
  }

  debug(PM, "Ctor: Marking GRUB loaded modules as reserved\n");
  //LastbutNotLeast: Mark Modules loaded by GRUB as reserved (i.e. pseudofs, etc)
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page = (ArchCommon::getModuleStartAddress(i) & 0x7FFFFFFF) / PAGE_SIZE;
    uint32 end_page = (ArchCommon::getModuleEndAddress(i) & 0x7FFFFFFF) / PAGE_SIZE;
    debug(PM, "Ctor: module: start_page: %d, end_page: %d, type: %d\n", start_page, end_page, type);
    for (size_t k = Min(start_page, number_of_pages_); k <= Min(end_page, number_of_pages_ - 1); ++k)
      page_usage_table_->setBit(k);
  }

  debug(PM, "Ctor: find lowest unreserved page\n");
  for (size_t p = lowest_unreserved_page_; p < number_of_pages_; ++p)
  {
    if (!page_usage_table_->getBit(p))
    {
      lowest_unreserved_page_ = p;
      break;
    }
  }
  debug(PM, "Ctor: Physical pages - free: %u used: %u total: %u\n", page_usage_table_->getNumFreeBits(),
        page_usage_table_->getNumBitsSet(), number_of_pages_);
  prenew_assert(lowest_unreserved_page_ < number_of_pages_);
  KernelMemoryManager::pm_ready_ = 1;
}

uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
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
  while (1)
  {
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
      ++lowest_unreserved_page_;
    lock_.release();

    if (found == 0)
    {
      debug(PM, "PageManager::allocPPN: FATAL ERROR!\n");
      debug(PM, "PageManager::allocPPN: Out of phyiscal pages!\n");
      assert(found);
    }
    memset((void*)ArchMemory::getIdentAddressOfPPN(found), 0, PAGE_SIZE);
    return found;
  }
  return 0;
}

void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  lock_.acquire();
  if (page_number < lowest_unreserved_page_)
    lowest_unreserved_page_ = page_number;
  for (uint32 p = page_number; p < (page_number + page_size / PAGE_SIZE); ++p)
  {
    assert(page_usage_table_->getBit(p))
    page_usage_table_->unsetBit(p);
  }
  lock_.release();
}
