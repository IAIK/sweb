/**
 * @file PageManager.cpp
 */

#include "mm/PageManager.h"
#include "mm/new.h"
#include "paging-definitions.h"
#include "arch_panic.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug_bochs.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "assert.h"

PageManager* PageManager::instance_=0;

extern void* kernel_end_address;

void PageManager::createPageManager()
{

  if (instance_)
    return;

  instance_ = new PageManager();
}

PageManager::PageManager()
{
  number_of_pages_ = 0;
  lowest_unreserved_page_ = 256; //physical memory <1MiB is reserved
  lock_=0;

  uint32 i=0,k=0;
  uint32 num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  pointer start_address=0, end_address=0;
  uint32 highest_address_below_1gig=0,type=0;

  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    debug(PM,"Ctor: usable memory region from physical %x to %x of type %d\n", start_address, end_address,type);
    if (type==1) 
      highest_address_below_1gig = Max(highest_address_below_1gig, Min(end_address, 256*1024*PAGE_SIZE));
  }
  //we can have a max of 1 GiB Memory (256*1024*4k)

  number_of_pages_ = highest_address_below_1gig / PAGE_SIZE;
  debug(PM,"Ctor: Number of physical pages: %d\n",number_of_pages_);

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

    for (k=Max(start_page, lowest_unreserved_page_); k<Min(end_page, 1024*256);++k)
    {
      page_usage_table_[k] = PAGE_FREE;
    }
  }

  //some of the useable memory regions are already in use by the kernel (propably the first 1024 pages)
  //therefore, mark as reserved everything >2gb und <3gb already used in PageDirectory 
  debug(PM,"Ctor: Marking stuff mapped in above 2 and < 3 gig as used\n");
  for (i=1024*512; i<1024*768;++i)
  {
    uint32 physical_page=0;
    uint32 pte_page=0;
    uint32 this_page_size = ArchMemory::getPhysicalPageOfVirtualPageInKernelMapping(i,&physical_page,&pte_page);
    if (this_page_size > 0)
    {
      //debug(PM,"Ctor: mark reserved: vp: %d, pp: %d, pptep: %d, page_size: %d\n",i,physical_page,pte_page,this_page_size);
      //our bitmap only knows 4k pages for now
      uint32 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint32 p=0;p<num_4kpages;++p)
        if (physical_page*num_4kpages + p < number_of_pages_)
          page_usage_table_[physical_page*num_4kpages + p] = PAGE_RESERVED;
      i+=(num_4kpages-1); //+0 in most cases
      if (num_4kpages == 1 && i % 1024 == 0)
        page_usage_table_[pte_page] = PAGE_RESERVED;
    }
  }
  
  debug(PM,"Ctor: Marking GRUB loaded modules as reserved\n");
  //LastbutNotLeast: Mark Modules loaded by GRUB as reserved (i.e. pseudofs, etc)
  for (i=0; i<ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page=( ArchCommon::getModuleStartAddress(i) - 3U*1024U*1024U*1024U ) / PAGE_SIZE;
    uint32 end_page=( ArchCommon::getModuleEndAddress(i) - 3U*1024U*1024U*1024U ) / PAGE_SIZE;
    for (k = start_page; k <= end_page; ++k)
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
  assert(lowest_unreserved_page_ >= 1024);
  assert(lowest_unreserved_page_ < number_of_pages_);
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

  if (lock_)
    lock_->acquire();

  //first 1024 pages are the 4MiB for Kernel Space
  for (uint32 p=lowest_unreserved_page_; p<number_of_pages_; ++p)
  {
    if (page_usage_table_[p] == PAGE_FREE)
    {
      page_usage_table_[p] = type;
      if (lock_)
        lock_->release();
      return p;
    }
  }
  if (lock_)
    lock_->release();  
  arch_panic((uint8*) "PageManager: Sorry, no more Pages Free !!!");
  return 0;
}

void PageManager::freePage(uint32 page_number)
{
  if (lock_)
    lock_->acquire();
  if (page_number >= lowest_unreserved_page_ && page_usage_table_[page_number] != PAGE_RESERVED)
    page_usage_table_[page_number] = PAGE_FREE;
  if (lock_)
    lock_->release();
}
