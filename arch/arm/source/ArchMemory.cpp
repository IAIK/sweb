/**
 * @file ArchMemory.cpp
 *
 */

#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"
#include "ArchCommon.h"
#include "PageManager.h"

#define PT_SIZE 1024
#define PD_SIZE 16384

#define PDE_SIZE_NONE 0
#define PDE_SIZE_PT 1
#define PDE_SIZE_PAGE 2

#define PAGE_4K_TO_1K(X) (X << 2)
#define PAGE_1K_TO_4K(X) (X >> 2)

//extern "C" uint32 kernel_page_directory_start;
extern "C" page_directory_entry kernel_page_directory_start;

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance()->getFreePhysicalPage(PAGE_4_PAGES_16K_ALIGNED);
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Got new Page no. %x\n",page_dir_page_ );

  page_directory_entry *new_page_directory = (page_directory_entry*) getIdentAddressOfPPN(page_dir_page_);

  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PD_SIZE);
  for (uint32 p = 8; p < PAGE_DIR_ENTRIES / 2; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.size = PDE_SIZE_NONE;
  }
  debug ( A_MEMORY,"ArchMemory::ArchMemory(): Initialised the page dir\n" );
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
  assert(page_directory[pde_vpn].pde4k.size != PDE_SIZE_PAGE);

  if (page_directory[pde_vpn].pde4k.size != PDE_SIZE_PT) return; // PT not present -> do nothing.

  for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
    if (pte_base[pte_vpn].size == 2)
      return; //not empty -> do nothing

  //else:
  page_directory[pde_vpn].pde4k.size = PDE_SIZE_NONE;
  PageManager::instance()->freePage(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_directory[pde_vpn].pde4k.size == PDE_SIZE_PAGE)
  {
    assert(false);
  }
  else if (page_directory[pde_vpn].pde4k.size == PDE_SIZE_PT)
  {
    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
    if (pte_base[pte_vpn].size == 2)
    {
      pte_base[pte_vpn].size = 0;
      PageManager::instance()->freePage(pte_base[pte_vpn].base);
    }
    checkAndRemovePT(pde_vpn);
  }
}

void ArchMemory::insertPT(uint32 pde_vpn, uint32 physical_page_table_page)
{
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  ArchCommon::bzero(getIdentAddressOfPPN(physical_page_table_page),PT_SIZE);
  page_directory[pde_vpn].pde4k.base = PAGE_4K_TO_1K(physical_page_table_page);
  page_directory[pde_vpn].pde4k.size = PDE_SIZE_PT;
}

void ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
//  kprintfd("ArchMemory::mapPage: v: %x to p: %x\n",virtual_page,physical_page);
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_size==PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pde4k.size == 0)
      insertPT(pde_vpn,PageManager::instance()->getFreePhysicalPage());

    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
    pte_base[pte_vpn].bufferable = 0;
    pte_base[pte_vpn].cachable = 0;
    pte_base[pte_vpn].permissions = user_access ? 3 : 1;
    pte_base[pte_vpn].reserved = 0;
    pte_base[pte_vpn].base = physical_page;
    pte_base[pte_vpn].size = 2;
  }
  else
    assert(false); // currently only 4K pages for the userspace
}


// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page directory %x\n",page_dir_page_ );
  page_directory_entry *page_directory = (page_directory_entry *) getIdentAddressOfPPN(page_dir_page_);
  for (uint32 pde_vpn=8; pde_vpn < PAGE_DIR_ENTRIES/2; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pde4k.size == PDE_SIZE_PAGE)
    {
      assert(false); // currently not used and not implemented
    }
    else if (page_directory[pde_vpn].pde4k.size == PDE_SIZE_PT)
    {
      page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
      for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
      {
        if (pte_base[pte_vpn].size == 2)
        {
          pte_base[pte_vpn].size = 0;
          PageManager::instance()->freePage(pte_base[pte_vpn].base);
        }
      }
      page_directory[pde_vpn].pde4k.size=PDE_SIZE_NONE;
      PageManager::instance()->freePage(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
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
  if (page_directory[pde_vpn].pde4k.size == PDE_SIZE_PAGE)
  {
    return true;
  }
  else if(page_directory[pde_vpn].pde4k.size == PDE_SIZE_PT)
  {
    page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(PAGE_1K_TO_4K(page_directory[pde_vpn].pde4k.base));
    if (pte_base[pte_vpn].size == 2)
    {
      return true;
    }
  }
  return false;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, uint32 *physical_page, uint32 *physical_pte_page)
{
  page_directory_entry *page_directory = &kernel_page_directory_start;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pde1m.size == PDE_SIZE_PAGE) // 1m page
  {
    *physical_page = page_directory[pde_vpn].pde1m.base;
    return 1024*1024;
  }
  else
    assert(false);
  return 0;
}
