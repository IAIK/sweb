/**
 * @file PageManager.cpp
 */

#include "mm/PageManager.h"
#include "mm/new.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug_bochs.h"
#include "console/kprintf.h"
#include "kernel/Scheduler.h"
#include "ArchInterrupts.h"
#include "assert.h"
#include "panic.h"

PageManager* PageManager::instance_=0;

extern void* kernel_end_address;

void PageManager::createPageManager()
{

  if (instance_)
    return;

  instance_ = new PageManager();
}

PageManager::PageManager() : lock_("PageManager::lock_")
{
  number_of_pages_ = 0;
  lowest_unreserved_page_ = 256; //physical memory <1MiB is reserved

  size_t i=0,k=0;
  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  pointer start_address=0, end_address=0,last_end_page=lowest_unreserved_page_;
  size_t highest_address_below_1gig=0,type=0,used_pages=0;

  //Determine Amount of RAM
  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    debug(PM,"Ctor: usable memory region from physical %x to %x of type %d\n", start_address, end_address, type);
    if (type==1)
      highest_address_below_1gig = Max(highest_address_below_1gig, Min(end_address & 0x7FFFFFFF, 256*1024*PAGE_SIZE));
  }
  //we can have a max of 1 GiB Memory (256*1024*4k)

  number_of_pages_ = highest_address_below_1gig / PAGE_SIZE;
  debug(PM,"Ctor: Number of physical pages: %d\n",number_of_pages_);

  //Determine RAM Used by Grub Modules
  for (i=0; i<ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page=( ArchCommon::getModuleStartAddress(i) & 0x3FFFFFFF ) / PAGE_SIZE;
    uint32 end_page=( ArchCommon::getModuleEndAddress(i) & 0x3FFFFFFF ) / PAGE_SIZE;

    assert(start_page <= last_end_page);
    if (start_page > end_page) {
      break;
    }
    used_pages += end_page - start_page + ((i>0 && end_page == last_end_page) ? 0 : 1);
    last_end_page = end_page;
  }
  lowest_unreserved_page_ = last_end_page;

  debug(PM,"Ctor: Pages used by Grub Modules %d\n",used_pages);

  //need at least 4 MiB for Kernel Memory + first physical MiB
  if (number_of_pages_ < 1025)
  {
    kprintfd("FATAL ERROR: Not enough Memory, Sweb needs at least %d KiB of RAM\n",(1025)*4096U);
    prenew_assert(false);
  }

  // maximum of 1 GiB physical memory supportet, just to be really sure ;->
  number_of_pages_ = Min(number_of_pages_,1024*256);

  //we use KMM now
  //note, the max size of our page_useage_table0 is 1MiB in theory (4GiB RAM) and 256KiB in practice (1GiB RAM)
  //(calculated with sizeof(puttype)=1)
  //currently we have 4MiB of kernel memory (1024 mapped pages starting at linear addr.: 2GiB)
  //if our kernel image becomes too large, the following command might fail
  page_usage_table_ = new puttype[number_of_pages_];

  // since we have gaps in the memory maps we can not give out everything
  // first mark everything as reserved, just to be sure
  debug(PM,"Ctor: Initializing page_usage_table_ with all pages reserved\n");
  for (i=0;i<number_of_pages_;++i)
  {
    page_usage_table_[i] = PAGE_RESERVED;
  }

  //now mark as free, everything that might be useable
  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    uint32 start_page = start_address / PAGE_SIZE;
    uint32 end_page = end_address / PAGE_SIZE;
    debug(PM,"Ctor: usable memory region: start_page: %d, end_page: %d, type: %d\n",start_page, end_page, type);
    if (start_page > 1024*256) //because max 1 gig of memory, see above
    {
      continue;
    }

    for (k=Max(start_page, lowest_unreserved_page_); k<Min(end_page, number_of_pages_);++k)
    {
      page_usage_table_[k] = PAGE_FREE;
    }
  }

  //some of the useable memory regions are already in use by the kernel (propably the first 1024 pages)
  //therefore, mark as reserved everything >2gb und <3gb already used in PageDirectory
  debug(PM,"Ctor: Marking stuff mapped in above 2 and < 3 gig as used\n");
//  uint32 last_page=0;
  for (i=ArchMemory::RESERVED_START; i<ArchMemory::RESERVED_END;++i)
  {
    size_t physical_page=0;
    size_t pte_page=0;
    size_t this_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(i,&physical_page,&pte_page);
    assert(this_page_size == 0 || this_page_size == PAGE_SIZE || this_page_size == PAGE_SIZE*PAGE_TABLE_ENTRIES);
    if (this_page_size > 0)
    {
      //our bitmap only knows 4k pages for now
      uint64 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint64 p=0;p<num_4kpages;++p)
        if (physical_page*num_4kpages + p < number_of_pages_)
          page_usage_table_[physical_page*num_4kpages + p] = PAGE_RESERVED;
      i+=(num_4kpages-1); //+0 in most cases
      if (num_4kpages == 1 && i % 1024 == 0 && pte_page < number_of_pages_)
        page_usage_table_[pte_page] = PAGE_RESERVED;
    }
  }

  debug(PM,"Ctor: Marking GRUB loaded modules as reserved\n");
  //LastbutNotLeast: Mark Modules loaded by GRUB as reserved (i.e. pseudofs, etc)
  for (i=0; i<ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page=( ArchCommon::getModuleStartAddress(i) & 0x7FFFFFFF ) / PAGE_SIZE;
    uint32 end_page=( ArchCommon::getModuleEndAddress(i) & 0x7FFFFFFF ) / PAGE_SIZE;
    debug(PM,"Ctor: module: start_page: %d, end_page: %d, type: %d\n",start_page, end_page, type);
    for (k = Min(start_page,number_of_pages_); k <= Min(end_page,number_of_pages_-1); ++k)
      page_usage_table_[k] = PAGE_RESERVED;
  }

  debug(PM,"Ctor: find lowest unreserved page\n");
  for (uint32 p=lowest_unreserved_page_; p<number_of_pages_; ++p)
  {
    if (page_usage_table_[p] == PAGE_FREE)
    {
      lowest_unreserved_page_=p;
      break;
    }
  }

  //will propably be 1024
  debug(PM,"Ctor: lowest_unreserved_page_=%d\n",lowest_unreserved_page_);
  prenew_assert(lowest_unreserved_page_ >= 512);
  prenew_assert(lowest_unreserved_page_ < number_of_pages_);
  debug(PM,"Ctor done\n");
}

uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}

//used by loader.cpp ArchMemory.cpp
uint32 PageManager::getFreePhysicalPage(uint32 type)
{
  if (type == PAGE_FREE || type == PAGE_RESERVED)  //what a stupid thing that would be to do
    return 0;

  lock_.acquire();

  //first 1024 pages are the 4MiB for Kernel Space
  for (uint32 p=lowest_unreserved_page_; p<number_of_pages_; ++p)
  {
    if (page_usage_table_[p] == PAGE_FREE)
    {
      if (type == PAGE_4_PAGES_16K_ALIGNED)
      {
        if ((p & 0x3) != 0)
          continue;
        if (page_usage_table_[p+1] == PAGE_FREE && page_usage_table_[p+2] == PAGE_FREE && page_usage_table_[p+3] == PAGE_FREE)
        {
          page_usage_table_[p] = type;
          page_usage_table_[p+1] = type;
          page_usage_table_[p+2] = type;
          page_usage_table_[p+3] = type;
          lowest_unreserved_page_ = p+4;
        }
        lock_.release();
        return p;
      }
      else
      {
        page_usage_table_[p] = type;
        lowest_unreserved_page_ = p+1;
        lock_.release();
        return p;
      }
    }
  }
  lock_.release();
  kpanict((uint8*) "PageManager: Sorry, no more Pages Free !!!");
  return 0;
}

void PageManager::freePage(uint32 page_number)
{
  lock_.acquire();
  if ( page_number < number_of_pages_ && page_usage_table_[page_number] != PAGE_RESERVED )
  {
    if ((page_number & 0x3) == 0 && page_usage_table_[page_number] == PAGE_4_PAGES_16K_ALIGNED
                                 && page_usage_table_[page_number + 1] == PAGE_4_PAGES_16K_ALIGNED
                                 && page_usage_table_[page_number + 2] == PAGE_4_PAGES_16K_ALIGNED
                                 && page_usage_table_[page_number + 3] == PAGE_4_PAGES_16K_ALIGNED)
    {
      page_usage_table_[page_number] = PAGE_FREE;
      page_usage_table_[page_number + 1] = PAGE_FREE;
      page_usage_table_[page_number + 2] = PAGE_FREE;
      page_usage_table_[page_number + 3] = PAGE_FREE;
    }
    page_usage_table_[page_number] = PAGE_FREE;
    if (page_number < lowest_unreserved_page_)
      lowest_unreserved_page_ = page_number;
  }
  lock_.release();
}
