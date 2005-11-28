//----------------------------------------------------------------------
//  $Id: ArchMemory.cpp,v 1.17 2005/11/28 10:28:01 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.cpp,v $
//  Revision 1.16  2005/09/24 13:30:20  btittelbach
//  4m page support
//
//  Revision 1.15  2005/09/21 19:26:24  btittelbach
//  support for 4m pages, part two
//
//  Revision 1.14  2005/09/21 18:38:43  btittelbach
//  ArchMemory differen page sizes part one
//
//  Revision 1.13  2005/09/03 19:02:54  btittelbach
//  PageManager++
//
//  Revision 1.12  2005/08/11 18:24:39  nightcreature
//  removed unused method physicalPageToKernelPointer
//
//  Revision 1.11  2005/07/26 17:45:25  nomenquis
//  foobar
//
//  Revision 1.10  2005/07/21 19:08:39  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.9  2005/07/05 20:22:56  btittelbach
//  some changes
//
//  Revision 1.8  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.7  2005/05/31 18:59:20  btittelbach
//  Special Address Check Function for Philip ;>
//
//  Revision 1.6  2005/05/25 08:27:48  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.5  2005/05/20 14:07:20  btittelbach
//  Redesign everything
//
//  Revision 1.4  2005/05/20 11:58:10  btittelbach
//  much much nicer UserProcess Page Management, but still things to do
//
//  Revision 1.3  2005/05/19 20:04:16  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.2  2005/05/19 19:35:30  btittelbach
//  ein bisschen Arch Memory
//
//  Revision 1.1  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//----------------------------------------------------------------------

#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"

extern "C" uint32 kernel_page_directory_start;

void ArchMemory::initNewPageDirectory(uint32 physical_page_to_use)
{
  page_directory_entry *new_page_directory = (page_directory_entry*) get3GBAdressOfPPN(physical_page_to_use);
  
  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE);
  for (uint32 p = 0; p < 512; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.present=0;
  }
}

void ArchMemory::checkAndRemovePTE(uint32 physical_page_directory_page, uint32 pde_lpn)
{
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
  assert(page_directory[pde_lpn].pde4m.use_4_m_pages == 0);
  for (uint32 pte_lpn=0; pte_lpn < PAGE_TABLE_ENTRIES; ++pte_lpn)
    if (pte_base[pte_lpn].present > 0)
      return; //not empty -> do nothing
    
  //else:
  page_directory[pde_lpn].pde4k.present = 0;
  PageManager::instance()->freePage(page_directory[pde_lpn].pde4k.page_table_base_address);
}

void ArchMemory::unmapPage(uint32 physical_page_directory_page, uint32 linear_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  uint32 pde_lpn = linear_page / PAGE_TABLE_ENTRIES;
  uint32 pte_lpn = linear_page % PAGE_TABLE_ENTRIES;
  
  if (page_directory[pde_lpn].pde4m.use_4_m_pages)
  {
    page_directory[pde_lpn].pde4m.present = 0;
    //PageManager manages Pages of size PAGE_SIZE only, so we have to free this_page_size/PAGE_SIZE Pages
    for (uint32 p=0;p<1024;++p)
      PageManager::instance()->freePage(page_directory[pde_lpn].pde4m.page_base_address*1024 + p);
  }
  {
    page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
    if (pte_base[pte_lpn].present)
    {
      pte_base[pte_lpn].present = 0;
      PageManager::instance()->freePage(pte_base[pte_lpn].page_base_address);
    }
    checkAndRemovePTE(physical_page_directory_page, pde_lpn);
  }
}

void ArchMemory::insertPTE(uint32 physical_page_directory_page, uint32 pde_lpn, uint32 physical_page_table_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  ArchCommon::bzero(get3GBAdressOfPPN(physical_page_table_page),PAGE_SIZE);
  page_directory[pde_lpn].pde4k.present = 1;
	page_directory[pde_lpn].pde4k.writeable = 1;
  page_directory[pde_lpn].pde4k.use_4_m_pages = 0;
  page_directory[pde_lpn].pde4k.page_table_base_address = physical_page_table_page; 
	page_directory[pde_lpn].pde4k.user_access = 1;
}

void ArchMemory::mapPage(uint32 physical_page_directory_page, uint32 linear_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
  kprintfd_nosleep("ArchMemory::mapPage: pys1 %x, pyhs2 %x\n",physical_page_directory_page, physical_page);
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  uint32 pde_lpn = linear_page / PAGE_TABLE_ENTRIES;
  uint32 pte_lpn = linear_page % PAGE_TABLE_ENTRIES;
  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_lpn].pde4k.present == 0)
    {
      //kprintfd("ArchMemory::mapPage: Need to add a pte for 0 %d %d %d\n",pde_lpn, linear_page, physical_page);
      insertPTE(physical_page_directory_page,pde_lpn,PageManager::instance()->getFreePhysicalPage());
    }
    page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
    //kprintfd("ArchMemory::mapPage: pte_base = %x\n",pte_base);
    pte_base[pte_lpn].present = 1;
    pte_base[pte_lpn].writeable = 1;
    pte_base[pte_lpn].user_access = user_access;
   // pte_base[pte_lpn].accessed = 0;
   // pte_base[pte_lpn].dirty = 0;
    pte_base[pte_lpn].page_base_address = physical_page;
  }
  else if ((page_size==PAGE_SIZE*1024) && (page_directory[pde_lpn].pde4m.present == 0))
  {
    page_directory[pde_lpn].pde4m.present = 1;
    page_directory[pde_lpn].pde4m.writeable = 1;
    page_directory[pde_lpn].pde4m.use_4_m_pages = 1;
    page_directory[pde_lpn].pde4m.page_base_address = physical_page; 
    page_directory[pde_lpn].pde4m.user_access = user_access;
  }
  else
    assert(false);
}


// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
void ArchMemory::freePageDirectory(uint32 physical_page_directory_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  for (uint32 pde_lpn=0; pde_lpn < PAGE_TABLE_ENTRIES/2; ++pde_lpn)
  {
    if (page_directory[pde_lpn].pde4k.present)
    {
      if (page_directory[pde_lpn].pde4m.use_4_m_pages)
      {
        page_directory[pde_lpn].pde4m.present=0;
          for (uint32 p=0;p<1024;++p)
            PageManager::instance()->freePage(page_directory[pde_lpn].pde4m.page_base_address*1024 + p);
      }
      else
      {
        page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
        for (uint32 pte_lpn=0; pte_lpn < PAGE_TABLE_ENTRIES; ++pte_lpn)
        {
          if (pte_base[pte_lpn].present)
          {
            pte_base[pte_lpn].present = 0;
            PageManager::instance()->freePage(pte_base[pte_lpn].page_base_address);
          }
        }
        page_directory[pde_lpn].pde4k.present=0;
        PageManager::instance()->freePage(page_directory[pde_lpn].pde4k.page_table_base_address);
      }
    }
  }
  PageManager::instance()->freePage(physical_page_directory_page);
}

bool ArchMemory::checkAddressValid(uint32 physical_page_directory_page, uint32 laddress_to_check)
{
  page_directory_entry *page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  uint32 linear_page = laddress_to_check / PAGE_SIZE;
  uint32 pde_lpn = linear_page / PAGE_TABLE_ENTRIES;
  uint32 pte_lpn = linear_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_lpn].pde4k.present)
  {
    if (page_directory[pde_lpn].pde4m.use_4_m_pages)
      return true;
    
    page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
    if (pte_base[pte_lpn].present)
      return true;
    else
      return false;
  }
  else
    return false;
}

uint32 ArchMemory::getPhysicalPageOfVirtualPageInKernelMapping(uint32 linear_page, uint32 *physical_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) &kernel_page_directory_start;
  //uint32 linear_page = laddress_to_check / PAGE_SIZE;
  uint32 pde_lpn = linear_page / PAGE_TABLE_ENTRIES;
  uint32 pte_lpn = linear_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_lpn].pde4k.present) //the present bit is the same for 4k and 4m
  {
    if (page_directory[pde_lpn].pde4m.use_4_m_pages)
    {
      *physical_page = page_directory[pde_lpn].pde4m.page_base_address;
      return (PAGE_SIZE*1024U);
    }
    else
    {
      page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(page_directory[pde_lpn].pde4k.page_table_base_address);
      if (pte_base[pte_lpn].present)
      {
        *physical_page = pte_base[pte_lpn].page_base_address;
        return PAGE_SIZE;
      }
      else
        return 0;
    }
  }
  else
    return 0;
}
