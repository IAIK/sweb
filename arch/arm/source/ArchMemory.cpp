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
  page_dir_page_ = PageManager::instance()->getFreePhysicalPage();
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Got new Page no. %x\n",page_dir_page_ );

  page_directory_entry *new_page_directory = (page_directory_entry*) getIdentAddressOfPPN(page_dir_page_);

  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE);
  for (uint32 p = 0; p < 512; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.present=0;
  }
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Initialised the page dir\n" );
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
  assert(page_directory[pde_vpn].pde4m.use_4_m_pages == 0);

  if (!page_directory[pde_vpn].pde4k.present) return; // PT not present -> do nothing.

  for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
    if (pte_base[pte_vpn].present > 0)
      return; //not empty -> do nothing

  //else:
  page_directory[pde_vpn].pde4k.present = 0;
  PageManager::instance()->freePage(page_directory[pde_vpn].pde4k.page_table_base_address);
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_directory[pde_vpn].pde4m.use_4_m_pages)
  {
    page_directory[pde_vpn].pde4m.present = 0;
    //PageManager manages Pages of size PAGE_SIZE only, so we have to free this_page_size/PAGE_SIZE Pages
    for (uint32 p=0;p<1024;++p)
      PageManager::instance()->freePage(page_directory[pde_vpn].pde4m.page_base_address*1024 + p);
  }
  else
  {
    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
    if (pte_base[pte_vpn].present)
    {
      pte_base[pte_vpn].present = 0;
      PageManager::instance()->freePage(pte_base[pte_vpn].page_base_address);
    }
    checkAndRemovePT(pde_vpn);
  }
}

void ArchMemory::insertPT(uint32 pde_vpn, uint32 physical_page_table_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_table_page),PAGE_SIZE);
  page_directory[pde_vpn].pde4k.writeable = 1;
  page_directory[pde_vpn].pde4k.use_4_m_pages = 0;
  page_directory[pde_vpn].pde4k.page_table_base_address = physical_page_table_page;
  page_directory[pde_vpn].pde4k.user_access = 1;
  page_directory[pde_vpn].pde4k.present = 1;
}

void ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
  //kprintfd("ArchMemory::mapPage: pys1 %x, pyhs2 %x\n",physical_page_directory_page, physical_page);
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pde4k.present == 0)
      insertPT(pde_vpn,PageManager::instance()->getFreePhysicalPage());

    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
    pte_base[pte_vpn].writeable = 1;
    pte_base[pte_vpn].user_access = user_access;
    pte_base[pte_vpn].page_base_address = physical_page;
    pte_base[pte_vpn].present = 1;
  }
  else if ((page_size==PAGE_SIZE*1024) && (page_directory[pde_vpn].pde4m.present == 0))
  {
    page_directory[pde_vpn].pde4m.writeable = 1;
    page_directory[pde_vpn].pde4m.use_4_m_pages = 1;
    page_directory[pde_vpn].pde4m.page_base_address = physical_page;
    page_directory[pde_vpn].pde4m.user_access = user_access;
    page_directory[pde_vpn].pde4m.present = 1;
  }
  else
    assert(false);
}


// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page directory %x\n",page_dir_page_ );
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  for (uint32 pde_vpn=0; pde_vpn < PAGE_TABLE_ENTRIES/2; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pde4k.present)
    {
      if (page_directory[pde_vpn].pde4m.use_4_m_pages)
      {
        page_directory[pde_vpn].pde4m.present=0;
          for (uint32 p=0;p<1024;++p)
            PageManager::instance()->freePage(page_directory[pde_vpn].pde4m.page_base_address*1024 + p);
      }
      else
      {
        page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
        for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
        {
          if (pte_base[pte_vpn].present)
          {
            pte_base[pte_vpn].present = 0;
            PageManager::instance()->freePage(pte_base[pte_vpn].page_base_address);
          }
        }
        page_directory[pde_vpn].pde4k.present=0;
        PageManager::instance()->freePage(page_directory[pde_vpn].pde4k.page_table_base_address);
      }
    }
  }
  PageManager::instance()->freePage(page_dir_page_);
}

bool ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pde4k.present)
  {
    if (page_directory[pde_vpn].pde4m.use_4_m_pages)
      return true;

    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
    if (pte_base[pte_vpn].present)
      return true;
    else
      return false;
  }
  else
    return false;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, uint32 *physical_page, uint32 *physical_pte_page)
{
  page_directory_entry *page_directory = &kernel_page_directory_start;
  //uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pde4k.present) //the present bit is the same for 4k and 4m
  {
    if (page_directory[pde_vpn].pde4m.use_4_m_pages)
    {
      *physical_page = page_directory[pde_vpn].pde4m.page_base_address;
      return (PAGE_SIZE*1024U);
    }
    else
    {
      if (physical_pte_page)
        *physical_pte_page = page_directory[pde_vpn].pde4k.page_table_base_address;
      page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
      if (pte_base[pte_vpn].present)
      {
        *physical_page = pte_base[pte_vpn].page_base_address;
        return PAGE_SIZE;
      }
      else
        return 0;
    }
  }
  else
    return 0;
}
