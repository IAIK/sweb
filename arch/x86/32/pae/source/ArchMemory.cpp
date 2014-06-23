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
extern "C" PageDirPointerTableEntry kernel_page_directory_pointer_table;

ArchMemory::ArchMemory() : page_dir_pointer_table_((PageDirPointerTableEntry*) (((uint32) page_dir_pointer_table_space_ + 0x20) & (~0x1F)))
{

  bzero(page_dir_pointer_table_,sizeof(PageDirPointerTableEntry) * PAGE_DIRECTORY_POINTER_TABLE_ENTRIES);
  page_dir_pointer_table_[0].present = 0; // will be created on demand
  page_dir_pointer_table_[1].present = 0; // will be created on demand
  page_dir_pointer_table_[2] = (&kernel_page_directory_pointer_table)[2]; // kernel
  page_dir_pointer_table_[3] = (&kernel_page_directory_pointer_table)[3]; // 1:1 mapping
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Initialised the page dir pointer table\n" );
}

void ArchMemory::checkAndRemovePT(uint32 physical_page_directory_page, uint32 pde_vpn)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(physical_page_directory_page);
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
  RESOLVEMAPPING(page_dir_pointer_table_,virtual_page);

  if (page_dir_pointer_table_[pdpte_vpn].present)
  {
    if (page_directory[pde_vpn].page.size)
    {
      page_directory[pde_vpn].page.present = 0;
      //PageManager manages Pages of size PAGE_SIZE only, so we have to free this_page_size/PAGE_SIZE Pages
      for (uint32 p=0;p<PAGE_TABLE_ENTRIES;++p)
        PageManager::instance()->freePage(page_directory[pde_vpn].page.page_ppn*1024 + p);
    }
    else
    {
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      if (pte_base[pte_vpn].present)
      {
        pte_base[pte_vpn].present = 0;
        PageManager::instance()->freePage(pte_base[pte_vpn].page_ppn);
      }
      checkAndRemovePT(page_dir_pointer_table_[pdpte_vpn].page_directory_ppn, pde_vpn);
    }
  }
}

void ArchMemory::insertPD(uint32 pdpt_vpn, uint32 physical_page_directory_page)
{
  kprintfd("insertPD: pdpt %x pdpt_vpn %x physical_page_table_page %x\n",page_dir_pointer_table_,pdpt_vpn,physical_page_directory_page);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_directory_page),PAGE_SIZE);
  ArchCommon::bzero((pointer)(page_dir_pointer_table_ + pdpt_vpn),sizeof(PageDirPointerTableEntry));
  page_dir_pointer_table_[pdpt_vpn].page_directory_ppn = physical_page_directory_page;
  page_dir_pointer_table_[pdpt_vpn].present = 1;
}

void ArchMemory::insertPT(PageDirEntry* page_directory, uint32 pde_vpn, uint32 physical_page_table_page)
{
  kprintfd("insertPT: page_directory %x pde_vpn %x physical_page_table_page %x\n",page_directory,pde_vpn,physical_page_table_page);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_table_page),PAGE_SIZE);
  ArchCommon::bzero((pointer)(page_directory + pde_vpn),sizeof(PageDirPointerTableEntry));
  page_directory[pde_vpn].pt.writeable = 1;
  page_directory[pde_vpn].pt.size = 0;
  page_directory[pde_vpn].pt.page_table_ppn = physical_page_table_page;
  page_directory[pde_vpn].pt.user_access = 1;
  page_directory[pde_vpn].pt.present = 1;
}

void ArchMemory::mapPage(uint32 virtual_page,
    uint32 physical_page, uint32 user_access, uint32 page_size)
{
  RESOLVEMAPPING(page_dir_pointer_table_,virtual_page);

  if (page_dir_pointer_table_[pdpte_vpn].present == 0)
  {
    uint32 ppn = PageManager::instance()->getFreePhysicalPage();
    page_directory = (PageDirEntry*) getIdentAddressOfPPN(ppn);
    insertPD(pdpte_vpn, ppn);
  }

  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pt.present == 0)
      insertPT(page_directory,pde_vpn,PageManager::instance()->getFreePhysicalPage());

    PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
    pte_base[pte_vpn].writeable = 1;
    pte_base[pte_vpn].user_access = user_access;
    pte_base[pte_vpn].page_ppn = physical_page;
    pte_base[pte_vpn].present = 1;
  }
  else if ((page_size==PAGE_SIZE*PAGE_TABLE_ENTRIES) && (page_directory[pde_vpn].page.present == 0))
  {
    page_directory[pde_vpn].page.writeable = 1;
    page_directory[pde_vpn].page.size = 1;
    page_directory[pde_vpn].page.page_ppn = physical_page;
    page_directory[pde_vpn].page.user_access = user_access;
    page_directory[pde_vpn].page.present = 1;
  }
  else
    assert(false);
}

ArchMemory::~ArchMemory()
{
  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page dir pointer table %x\n",page_dir_pointer_table_ );
  freePageDirectory(page_dir_pointer_table_[0].page_directory_ppn); // 0-1 GiB
  freePageDirectory(page_dir_pointer_table_[1].page_directory_ppn); // 1-2 GiB
}

void ArchMemory::freePageDirectory(uint32 physical_page_directory_page)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(physical_page_directory_page);
  for (uint32 pde_vpn=0; pde_vpn < PAGE_DIRECTORY_ENTRIES; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      if (page_directory[pde_vpn].page.size)
      {
        page_directory[pde_vpn].page.present=0;
          for (uint32 p=0;p<1024;++p)
            PageManager::instance()->freePage(page_directory[pde_vpn].page.page_ppn*1024 + p);
      }
      else
      {
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
  }
  PageManager::instance()->freePage(physical_page_directory_page);
}

bool ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  RESOLVEMAPPING(page_dir_pointer_table_, vaddress_to_check / PAGE_SIZE);
  if (page_dir_pointer_table_[pdpte_vpn].present)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      if (page_directory[pde_vpn].page.size)
        return true;

      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      if (pte_base[pte_vpn].present)
        return true;
    }
  }
  return false;
}

uint32 ArchMemory::get_PAddr_Of_VAddr_In_KernelMapping(uint32 virtual_addr)
{
  size_t physical_addr;
  uint32 page_size = get_PPN_Of_VPN_In_KernelMapping(virtual_addr / PAGE_SIZE, &physical_addr);
  return physical_addr * page_size + (virtual_addr % PAGE_SIZE);
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, size_t *physical_page, uint32 *physical_pte_page)
{
  PageDirPointerTableEntry *pdpt = &kernel_page_directory_pointer_table;
  RESOLVEMAPPING(pdpt, virtual_page);
  if (pdpt[pdpte_vpn].present)
  {
    if (page_directory[pde_vpn].pt.present)
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
      }
    }
  }
  return 0;
}
