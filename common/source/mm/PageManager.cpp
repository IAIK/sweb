//----------------------------------------------------------------------
//   $Id: PageManager.cpp,v 1.7 2005/04/26 10:58:15 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.cpp,v $
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

PageManager* PageManager::instance_=0;

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
   
extern void* kernel_end_address;
    
pointer PageManager::createPageManager(pointer next_usable_address)
{
  if (instance_)
    return 1;

  // ok, our poor page manager can't use neither new nor something like kmalloc
  // poor guy
  // we know where the kernel ends, and this is good, we'll simply append ourselves after
  // the end of the kernel
  // me likes hacks ;)
  instance_ = new ((void*)next_usable_address) PageManager(next_usable_address+sizeof(PageManager));

  next_usable_address += sizeof(PageManager) + instance_->getSizeOfMemoryUsed();
  print(next_usable_address);
  print(next_usable_address / 1024);
  print(next_usable_address - 1024*1024*1024*2);
  
  return next_usable_address;
}

PageManager::PageManager(pointer start_of_structure)
{
  number_of_pages_ = 0;
  page_usage_table_ = (uint32*)start_of_structure;
  
  uint32 i,k;
  uint32 num_mmaps = ArchCommon::getNumUseableMemoryRegions();
  print(num_mmaps);
  
  pointer start_address, end_address;
  uint32 type;
  
  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    if (type==1) number_of_pages_ = Max(number_of_pages_,end_address);
  }
  
  number_of_pages_ = number_of_pages_ / PAGE_SIZE;
  
  print(number_of_pages_);
  
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
  
  print (number_of_pages_for_structure);
  print (number_of_used_pages);
  print (number_of_free_pages);
  
  // since we have gaps in the memory maps we can not give out everything
  // first mark everything as reserverd, and then mark everything we actually
  // can use as free
  for (i=0;i<number_of_pages_;++i)
  {
    page_usage_table_[i] = PAGE_RESERVED;
  }
  print (11)
  
  for (i=0;i<num_mmaps;++i)
  {
    ArchCommon::getUsableMemoryRegion(i,start_address,end_address,type);
    start_address /= PAGE_SIZE;
    end_address /= PAGE_SIZE;
    print(start_address)
    print(end_address)
    if (start_address > 1024*256 || end_address > 1024*256)
    {
      print(777777777);
      continue;
    }
    for (k=start_address;k<end_address;++k)
    {
      page_usage_table_[k] = PAGE_FREE;
    }
    print (11+i)
  }    
  print (22)
  // next, the first 4 megs are allocated for the kernel
  for (i=0;i<1024;++i)
  {
    page_usage_table_[i] = PAGE_KERNEL;
  }
  print (12341234);
}

uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}

uint32 PageManager::getSizeOfMemoryUsed() const
{
  return number_of_pages_ * sizeof(uint32); 
}
