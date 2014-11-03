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
extern "C" PageDirEntry kernel_page_directory_start;

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance()->getFreePhysicalPage();
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Got new Page no. %x\n",page_dir_page_ );

  PageDirEntry *new_page_directory = (PageDirEntry*) getIdentAddressOfPPN(page_dir_page_);

  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE);
  for (uint32 p = 0; p < 512; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pt.present=0;
  }
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Initialised the page dir\n" );
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  assert(page_directory[pde_vpn].page.size == 0);

  if (!page_directory[pde_vpn].pt.present) return; // PT not present -> do nothing.

  for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
    if (pte_base[pte_vpn].present > 0)
      return; //not empty -> do nothing

  //else:
  page_directory[pde_vpn].pt.present = 0;
  PageManager::instance()->freePage(page_directory[pde_vpn].pt.page_table_ppn);
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  assert(!page_directory[pde_vpn].page.size); // only 4 KiB pages allowed
  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  if (pte_base[pte_vpn].present)
  {
    pte_base[pte_vpn].present = 0;
    PageManager::instance()->freePage(pte_base[pte_vpn].page_ppn);
  }
  checkAndRemovePT(pde_vpn);
}

void ArchMemory::insertPT(uint32 pde_vpn, uint32 physical_page_table_page)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_table_page),PAGE_SIZE);
  page_directory[pde_vpn].pt.writeable = 1;
  page_directory[pde_vpn].pt.size = 0;
  page_directory[pde_vpn].pt.page_table_ppn = physical_page_table_page;
  page_directory[pde_vpn].pt.user_access = 1;
  page_directory[pde_vpn].pt.present = 1;
}

void ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
  //kprintfd("ArchMemory::mapPage: pys1 %x, pyhs2 %x\n",physical_page_directory_page, physical_page);
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pt.present == 0)
      insertPT(pde_vpn,PageManager::instance()->getFreePhysicalPage());

    PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
    pte_base[pte_vpn].writeable = 1;
    pte_base[pte_vpn].user_access = user_access;
    pte_base[pte_vpn].page_ppn = physical_page;
    pte_base[pte_vpn].present = 1;
  }
  else
    assert(false);
}


// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page directory %x\n",page_dir_page_ );
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  for (uint32 pde_vpn=0; pde_vpn < PAGE_TABLE_ENTRIES/2; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      assert(!page_directory[pde_vpn].page.size); // only 4 KiB pages allowed
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
      {
        if (pte_base[pte_vpn].present)
        {
          pte_base[pte_vpn].present = 0;
          PageManager::instance()->freePage(pte_base[pte_vpn].page_ppn);
        }
      }
      page_directory[pde_vpn].pt.present=0;
      PageManager::instance()->freePage(page_directory[pde_vpn].pt.page_table_ppn);
    }
  }
  PageManager::instance()->freePage(page_dir_page_);
}

bool ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pt.present)
  {
    if (page_directory[pde_vpn].page.size)
      return true;

    PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
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
  PageDirEntry *page_directory = &kernel_page_directory_start;
  //uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pt.present) //the present bit is the same for 4k and 4m
  {
    if (page_directory[pde_vpn].page.size)
    {
      *physical_page = page_directory[pde_vpn].page.page_ppn;
      return (PAGE_SIZE*1024U);
    }
    else
    {
      if (physical_pte_page)
        *physical_pte_page = page_directory[pde_vpn].pt.page_table_ppn;
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      if (pte_base[pte_vpn].present)
      {
        *physical_page = pte_base[pte_vpn].page_ppn;
        return PAGE_SIZE;
      }
      else
        return 0;
    }
  }
  else
    return 0;
}

uint32 ArchMemory::getRootOfPagingStructure()
{
  return page_dir_page_;
}

pointer ArchMemory::getIdentAddressOfPPN(uint32 ppn, uint32 page_size /* optional */)
{
  return (3U*1024U*1024U*1024U) + (ppn * page_size);
}
