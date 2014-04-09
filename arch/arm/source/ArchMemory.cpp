/**
 * @file ArchMemory.cpp
 *
 */

#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"
#include "ArchCommon.h"
#include "PageManager.h"

//extern "C" uint32 kernel_page_directory_start;
extern "C" page_directory_entry kernel_page_directory_start;

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance()->getFreePhysicalPage(PAGE_4_PAGES_16K_ALIGNED);
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Got new Page no. %x\n",page_dir_page_ );

  page_directory_entry *new_page_directory = (page_directory_entry*) getIdentAddressOfPPN(page_dir_page_);

  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE*4);
  for (uint32 p = 0; p < 2048; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.size=0;
  }
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Initialised the page dir\n" );
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  assert(false);
//  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
//  page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
//  assert(page_directory[pde_vpn].pde4m.use_4_m_pages == 0);
//
//  if (!page_directory[pde_vpn].pde4k.present) return; // PT not present -> do nothing.
//
//  for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
//    if (pte_base[pte_vpn].present > 0)
//      return; //not empty -> do nothing
//
//  //else:
//  page_directory[pde_vpn].pde4k.present = 0;
//  PageManager::instance()->freePage(page_directory[pde_vpn].pde4k.page_table_base_address);
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  assert(false);
//  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
//  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
//  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
//
//  if (page_directory[pde_vpn].pde4m.use_4_m_pages)
//  {
//    page_directory[pde_vpn].pde4m.present = 0;
//    //PageManager manages Pages of size PAGE_SIZE only, so we have to free this_page_size/PAGE_SIZE Pages
//    for (uint32 p=0;p<1024;++p)
//      PageManager::instance()->freePage(page_directory[pde_vpn].pde4m.page_base_address*1024 + p);
//  }
//  else
//  {
//    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
//    if (pte_base[pte_vpn].present)
//    {
//      pte_base[pte_vpn].present = 0;
//      PageManager::instance()->freePage(pte_base[pte_vpn].page_base_address);
//    }
//    checkAndRemovePT(pde_vpn);
//  }
}

void ArchMemory::insertPT(uint32 pde_vpn, uint32 physical_page_table_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_table_page),PAGE_SIZE);
  page_directory[pde_vpn].pde4k.base = physical_page_table_page;
  page_directory[pde_vpn].pde4k.size = 1;
}

void ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
  kprintfd("ArchMemory::mapPage: pys1 %x, pyhs2 %x\n",page_dir_page_, physical_page);
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pde4k.size == 0)
      insertPT(pde_vpn,PageManager::instance()->getFreePhysicalPage());

    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.base);
    pte_base[pte_vpn].bufferable = 0;
    pte_base[pte_vpn].cachable = 0;
    pte_base[pte_vpn].permissions = user_access ? 3 : 1;
    pte_base[pte_vpn].reserved = 0;
    pte_base[pte_vpn].base = physical_page;
    pte_base[pte_vpn].size = 2;
  }
  else if ((page_size==PAGE_SIZE*256) && (page_directory[pde_vpn].pde1m.size == 0))
  {
    page_directory[pde_vpn].pde1m.bufferable = 0;
    page_directory[pde_vpn].pde1m.cachable = 0;
    page_directory[pde_vpn].pde1m.domain = 0;
    page_directory[pde_vpn].pde1m.reserved_1 = 0;
    page_directory[pde_vpn].pde1m.reserved_2 = 0;
    page_directory[pde_vpn].pde1m.reserved_3 = 0;
    page_directory[pde_vpn].pde1m.base = physical_page;
    page_directory[pde_vpn].pde1m.permissions = user_access ? 3 : 1;
    page_directory[pde_vpn].pde1m.size = 2;
  }
  else
    assert(false);
}


// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  assert(false);
//  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page directory %x\n",page_dir_page_ );
//  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
//  for (uint32 pde_vpn=0; pde_vpn < PAGE_TABLE_ENTRIES/2; ++pde_vpn)
//  {
//    if (page_directory[pde_vpn].pde4k.present)
//    {
//      if (page_directory[pde_vpn].pde4m.use_4_m_pages)
//      {
//        page_directory[pde_vpn].pde4m.present=0;
//          for (uint32 p=0;p<1024;++p)
//            PageManager::instance()->freePage(page_directory[pde_vpn].pde4m.page_base_address*1024 + p);
//      }
//      else
//      {
//        page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
//        for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
//        {
//          if (pte_base[pte_vpn].present)
//          {
//            pte_base[pte_vpn].present = 0;
//            PageManager::instance()->freePage(pte_base[pte_vpn].page_base_address);
//          }
//        }
//        page_directory[pde_vpn].pde4k.present=0;
//        PageManager::instance()->freePage(page_directory[pde_vpn].pde4k.page_table_base_address);
//      }
//    }
//  }
//  PageManager::instance()->freePage(page_dir_page_);
}

bool ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  assert(false);
//  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
//  uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
//  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
//  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
//  if (page_directory[pde_vpn].pde4k.present)
//  {
//    if (page_directory[pde_vpn].pde4m.use_4_m_pages)
//      return true;
//
//    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
//    if (pte_base[pte_vpn].present)
//      return true;
//    else
//      return false;
//  }
//  else
//    return false;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, uint32 *physical_page, uint32 *physical_pte_page)
{
  page_directory_entry *page_directory = &kernel_page_directory_start;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pde1m.size == 2) // 1m page
  {
    *physical_page = page_directory[pde_vpn].pde1m.base;
    return 1024*1024;
  }
  else
    return 0;
}
