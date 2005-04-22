//----------------------------------------------------------------------
//   $Id: PageManager.cpp,v 1.2 2005/04/22 19:43:04 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.cpp,v $
//  Revision 1.1  2005/04/20 21:35:33  nomenquis
//  started to implement page manager
//
//----------------------------------------------------------------------

#include "mm/PageManager.h"
#include "mm/new.h"
#include "paging-definitions.h"
#include "arch_panic.h"

PageManager* PageManager::instance_=0;

extern void* kernel_end_address;
uint32 PageManager::createPageManager(uint32 const &number_of_pages)
{
  if (instance_)
    return 1;

  // ok, our poor page manager can't use neither new nor something like kmalloc
  // poor guy
  // we know where the kernel ends, and this is good, we'll simply append ourselves after
  // the end of the kernel
  // me likes hacks ;)
  void *our_page_manager_address = &kernel_end_address;
  instance_ = new (our_page_manager_address) PageManager(number_of_pages);

  return 0;
}

PageManager::PageManager(uint32 number_of_pages)
{
  // we support no more than a total maximum of 1 gig memory
  if (number_of_pages > 1024*256)
  {
    number_of_pages = 1024*256;
  }
  
  size_t length_of_structure = number_of_pages * sizeof(uint32);
  size_t number_of_pages_for_structure = length_of_structure / PAGE_SIZE;
  size_t number_of_used_pages = ((uint32)&kernel_end_address)-1024*1024*1024*2;
  size_t number_of_free_pages = 4*1024 - number_of_used_pages;
  
  if (number_of_free_pages < length_of_structure + 1)
  {
    // paaaaaaaaaaaaaaniiiiiiiiiiiiiiiiiiiiic
    arch_panic((uint8*)"Unable to initialise PageManager, not enough kernel memory for data structures");
  }
  
  // here we should definitely use our über cool schubba algorithm to find out what to do
  
  
}
