//----------------------------------------------------------------------
//   $Id: PageManager.cpp,v 1.16 2005/09/27 21:24:43 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.cpp,v $
//  Revision 1.15  2005/09/21 19:54:04  btittelbach
//  +readability
//  -warnings
//
//  Revision 1.14  2005/09/21 19:49:14  btittelbach
//  PageManager now understands preallocated 4m pages
//
//  Revision 1.13  2005/09/21 17:01:12  nomenquis
//  updates
//
//  Revision 1.12  2005/09/21 15:50:55  nomenquis
//  fixed some bugs when having a vesa console
//
//  Revision 1.11  2005/09/21 14:00:51  nomenquis
//  fixed bug for machines with >1gig of memory
//
//  Revision 1.10  2005/09/03 19:02:54  btittelbach
//  PageManager++
//
//  Revision 1.9  2005/08/11 18:28:10  nightcreature
//  changed define of evil print(x) depending on platform xen or x86
//
//  Revision 1.8  2005/05/19 15:43:43  btittelbach
//  Ansätze für eine UserSpace Verwaltung
//
//  Revision 1.7  2005/04/26 10:58:15  nomenquis
//  and now it really works
//
//  Revision 1.6  2005/04/25 23:09:18  nomenquis
//  fubar 2
//
//  Revision 1.5  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.4  2005/04/23 12:52:26  nomenquis
//  fixes
//
//  Revision 1.2  2005/04/22 19:43:04  nomenquis
//   more poison added
//
//  Revision 1.1  2005/04/20 21:35:33  nomenquis
//  started to implement page manager
//
//----------------------------------------------------------------------

#include "mm/PageManager.h"
#include "mm/new.h"
#include "paging-definitions.h"
#include "arch_panic.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
//#include "hypervisor.h"
#include "debug_bochs.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "assert.h"

PageManager* PageManager::instance_=0;
  
extern void* kernel_end_address;
    
pointer PageManager::createPageManager(pointer next_usable_address)
{
  //next_usable_address is kernel_end_address defined in linker script

  
  if (instance_)
    return 1;

  // ok, our poor page manager can't use neither new nor something like kmalloc
  // poor guy
  // we know where the kernel ends, and this is good, we'll simply append ourselves after
  // the end of the kernel
  // me likes hacks ;)
  instance_ = new ((void*)next_usable_address) PageManager(next_usable_address+sizeof(PageManager));

  next_usable_address += sizeof(PageManager) + instance_->getSizeOfMemoryUsed();
  
  return next_usable_address;
}

PageManager::PageManager(pointer start_of_structure)
{
  number_of_pages_ = 0;
  page_usage_table_ = (puttype*)start_of_structure;
  lock_=0;
  
  uint32 i,k;
  uint32 num_mmaps = ArchCommon::getNumUseableMemoryRegions();
  //print(num_mmaps);
  
  pointer start_address, end_address;
  uint32 type;
  
  for (i=0;i<num_mmaps;++i)
  {
    writeLine2Bochs((uint8*)"PM: Managing region\n");
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    kprintfd("Have a memory region from page %d to page %d of type %d\n", start_address, end_address,type);
    if (type==1) number_of_pages_ = Max(number_of_pages_,(Min(end_address,256*1024*PAGE_SIZE)));
  }
  
  number_of_pages_ = number_of_pages_ / PAGE_SIZE;
  kprintfd("Number of physical pages: %d\n",number_of_pages_);
  //print(number_of_pages_);
  
  // max of 1 gig memory supportet
  number_of_pages_ = Min(number_of_pages_,1024*256);

  size_t length_of_structure = number_of_pages_ * sizeof(puttype);
  size_t number_of_pages_for_structure = length_of_structure / PAGE_SIZE;
  
  size_t number_of_used_pages = (((uint32)&kernel_end_address)-1024*1024*1024*2) / PAGE_SIZE; 
  size_t number_of_free_pages = 1024 - number_of_used_pages;

 
  if (number_of_free_pages < number_of_pages_for_structure)
  {
    arch_panic((uint8*)"PM: Error, not enough memory for pages");
  }

  //print (number_of_pages_for_structure);
  //print (number_of_used_pages);
  //print (number_of_free_pages);
  
  // since we have gaps in the memory maps we can not give out everything
  // first mark everything as reserverd, and then mark everything we actually
  // can use as free
  writeLine2Bochs((uint8*)"PM: Marking pages used by kernel as reserved\n");
  for (i=0;i<number_of_pages_;++i)
//  for (i=0;i<10;++i)
  {
    page_usage_table_[i] = PAGE_RESERVED;
  }

    /* do nothing */
    //for ( ; ; ) HYPERVISOR_yield();

   
  for (i=0;i<num_mmaps;++i)
  {
    writeLine2Bochs((uint8*)"PM: Entering map for \n");
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    start_address /= PAGE_SIZE;
    end_address /= PAGE_SIZE;
    //print(start_address)
    //print(end_address)
    if (start_address > 1024*256) //becaue max 1 gig of memory?, see above
    {
      //print(777777777);
      continue;
    }
    
    for (k=start_address;k<Min(end_address,1024*256);++k)
    {
      page_usage_table_[k] = PAGE_FREE;
    }
    writeLine2Bochs((uint8*)"PM: Exiting map for \n");
  }    
  
  //mark as used everything >2gb und <3gb already used in PageDirectory
  writeLine2Bochs((uint8*)"PM: Marking stuff mapped in above 2 and < 3 gig as used\n");
  for (i=1024*512; i<1024*764; ++i) // only 764 because from 764 on we have the svga framebuffer in 4m pages
  {
    uint32 physical_page=0;
    uint32 this_page_size = ArchMemory::getPhysicalPageOfVirtualPageInKernelMapping(i,&physical_page);
    if (this_page_size > 0)
    {
      //our bitmap only knows 4k pages for now
      uint32 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint32 p=0;p<num_4kpages;++p)
        if (physical_page*num_4kpages + p < number_of_pages_)
          page_usage_table_[physical_page*num_4kpages + p] = PAGE_RESERVED;
    }
  }
  writeLine2Bochs((uint8*)"PM: done\n");
  //Mark Modules loaded by GRUB as used
  for (i=0; i<ArchCommon::getNumModules(); ++i)
  {
    uint32 start_page=( ArchCommon::getModuleStartAddress(i) - 3U*1024U*1024U*1024U ) / PAGE_SIZE;
    uint32 end_page=( ArchCommon::getModuleEndAddress(i) - 3U*1024U*1024U*1024U ) / PAGE_SIZE;
    for (k = start_page; k <= end_page; ++k)
      page_usage_table_[k] = PAGE_RESERVED;
  }
  
}

uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}

uint32 PageManager::getSizeOfMemoryUsed() const
{
  return number_of_pages_ * sizeof(puttype); 
}

//used by loader.cpp ArchMemory.cpp UerProcess.cpp
uint32 PageManager::getFreePhysicalPage(uint32 type)
{
  if (type == PAGE_FREE)  //what a stupid thing that would be to do
    return 0;
 
  if (lock_)
    lock_->acquire();
  
  for (uint32 p=1024; p<number_of_pages_; ++p)  
  {
    if (page_usage_table_[p] == PAGE_FREE)
    {
      page_usage_table_[p] = type;
      if (lock_)
        lock_->release();
      return p;
    }
  }
  kprintfd("I have %d pages \n",number_of_pages_); 
  arch_panic((uint8*) "PageManager: Sorry, no more Pages Free !!!");
  if (lock_)
    lock_->release();  
  return 0;
}

void PageManager::freePage(uint32 page_number) 
{
  if (lock_)
    lock_->acquire();
  if (page_number >= 1024 && page_usage_table_[page_number] != PAGE_RESERVED)
    page_usage_table_[page_number] = PAGE_FREE;
  if (lock_)
    lock_->release(); 
}
