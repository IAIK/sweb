//----------------------------------------------------------------------
//   $Id: PageManager.cpp,v 1.11 2005/09/21 14:00:51 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.cpp,v $
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

#ifndef isXenBuild
static uint32 fb_start = 0;
static char* fb = (char*)0xC00B8100;

#define print(x)     fb_start += 2; \
    { \
      uint32 divisor; \
      uint32 current; \
      uint32 remainder; \
      current = (uint32)x; \
      divisor = 1000000000; \
      while (divisor > 0) \
      { \
        remainder = current % divisor; \
        current = current / divisor; \
        \
        fb[fb_start++] = (uint8)current + '0' ; \
        fb[fb_start++] = 0x9f ; \
    \
        divisor = divisor / 10; \
        current = remainder; \
      }      \
      uint32 blubba;\
      uint32 asf;\
      for (asf=0;asf<1;++asf)\
        ++blubba;\
    }
#else
  #define print(x) 
#endif
//the following is possible but not so good as it is printing on the console for ages
// #else
//   #include "console/kprintf.h"
//   #define print(x) kprintf("%x ",x);
// #endif


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
  //print(next_usable_address);
  //print(next_usable_address / 1024);
  //print(next_usable_address - 1024*1024*1024*2);
  
  return next_usable_address;
}

PageManager::PageManager(pointer start_of_structure)
{
  number_of_pages_ = 0;
  page_usage_table_ = (uint32*)start_of_structure;
  
  uint32 i,k;
  uint32 num_mmaps = ArchCommon::getNumUseableMemoryRegions();
  //print(num_mmaps);
  
  pointer start_address, end_address;
  uint32 type;
  
  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    if (type==1) number_of_pages_ = Max(number_of_pages_,end_address);
  }
  
  number_of_pages_ = number_of_pages_ / PAGE_SIZE;
  
  //print(number_of_pages_);
  
  // max of 1 gig memory supportet
  number_of_pages_ = Min(number_of_pages_,1024*256);

  size_t length_of_structure = number_of_pages_ * sizeof(uint32);
  size_t number_of_pages_for_structure = length_of_structure / PAGE_SIZE;
  
  size_t number_of_used_pages = (((uint32)&kernel_end_address)-1024*1024*1024*2) / PAGE_SIZE; 
  size_t number_of_free_pages = 1024 - number_of_used_pages;

 
  if (number_of_free_pages < number_of_pages_for_structure)
  {
    arch_panic((uint8*)"Error, not enough memory for pages");
  }

  //print (number_of_pages_for_structure);
  //print (number_of_used_pages);
  //print (number_of_free_pages);
  
  // since we have gaps in the memory maps we can not give out everything
  // first mark everything as reserverd, and then mark everything we actually
  // can use as free
  for (i=0;i<number_of_pages_;++i)
//  for (i=0;i<10;++i)
  {
    page_usage_table_[i] = PAGE_RESERVED;
  }

    /* do nothing */
    //for ( ; ; ) HYPERVISOR_yield();

   
  for (i=0;i<num_mmaps;++i)
  {
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
  }    
  
  //mark as used everything >2gb und <3gb already used in PageDirectory
  for (i=1024*512; i<1024*768; ++i)
  {
    uint32 physical_page=0;
    if (ArchMemory::getPhysicalPageOfVirtualPageInKernelMapping(i,&physical_page))
      page_usage_table_[physical_page] = PAGE_RESERVED;
  }
  
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
  return number_of_pages_ * sizeof(uint32); 
}

//used by loader.cpp ArchMemory.cpp UerProcess.cpp
uint32 PageManager::getFreePhysicalPage(uint32 type)
{
  if (type == PAGE_FREE)  //what a stupid thing that would be to do
    return 0;
  
  for (uint32 p=1024; p<number_of_pages_; ++p)  //start beyond kernel pages
  {
    if (page_usage_table_[p] == PAGE_FREE)
    {
      page_usage_table_[p] = type;
      return p;
    }
  }
  arch_panic((uint8*) "PageManager: Sorry, no more Pages Free !!!");
  return 0;
}

void PageManager::freePage(uint32 page_number) 
{
  if (page_number >= 1024 && page_usage_table_[page_number] != PAGE_RESERVED)
    page_usage_table_[page_number] = PAGE_FREE;
}
