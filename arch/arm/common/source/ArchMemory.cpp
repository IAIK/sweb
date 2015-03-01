/**
 * @file ArchMemory.cpp
 *
 */

#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "offsets.h"
#include "kstring.h"

#define PT_SIZE 1024
#define PD_SIZE 16384

#define PDE_SIZE_NONE 0
#define PDE_SIZE_PT 1
#define PDE_SIZE_PAGE 2

#define PHYS_OFFSET_4K (LOAD_BASE / PAGE_SIZE)
#define PHYS_OFFSET_1M (PHYS_OFFSET_4K / PAGE_TABLE_ENTRIES)

PageDirEntry kernel_page_directory[PAGE_DIR_ENTRIES] __attribute__((aligned(0x4000))); // space for page directory
PageTableEntry kernel_page_tables[16 * PAGE_TABLE_ENTRIES] __attribute__((aligned(1024))); // space for 8 page tables

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance()->allocPPN(4 * PAGE_SIZE);
  debug(A_MEMORY, "ArchMemory::ArchMemory(): Got new Page no. %x\n", page_dir_page_);

  PageDirEntry *new_page_directory = (PageDirEntry*) getIdentAddressOfPPN(page_dir_page_);
  memcpy((void*) new_page_directory, (const void*) kernel_page_directory, PD_SIZE);
  for (uint32 p = 8; p < PAGE_DIR_ENTRIES / 2; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pt.size = PDE_SIZE_NONE;
  }
  debug(A_MEMORY, "ArchMemory::ArchMemory(): Initialised the page dir\n");
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
  assert(page_directory[pde_vpn].pt.size != PDE_SIZE_PAGE);

  if (page_directory[pde_vpn].pt.size != PDE_SIZE_PT)
    return; // PT not present -> do nothing.

  for (uint32 pte_vpn = 0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
    if (pte_base[pte_vpn].size == 2)
      return; //not empty -> do nothing

  //else:
  page_directory[pde_vpn].pt.size = PDE_SIZE_NONE;
  pt_ppns_.push_back((page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K) * 4 + page_directory[pde_vpn].pt.offset);
  for (size_t i = 0; i < 4; ++i)
    if (ustl::find(pt_ppns_.begin(), pt_ppns_.end(),page_directory[pde_vpn].pt.pt_ppn * 4 + i) == pt_ppns_.end())
      return;
  PageManager::instance()->freePPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K);
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_directory[pde_vpn].pt.size == PDE_SIZE_PAGE)
  {
    assert(false);
  }
  else if (page_directory[pde_vpn].pt.size == PDE_SIZE_PT)
  {
    PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
    if (pte_base[pte_vpn].size == 2)
    {
      pte_base[pte_vpn].size = 0;
      PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn - PHYS_OFFSET_4K);
    }
    checkAndRemovePT(pde_vpn);
  }
}

void ArchMemory::insertPT(uint32 pde_vpn)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 physical_page_table_page = 0;
  uint32 offset = 0;
  if (pt_ppns_.size() > 0)
  {
    physical_page_table_page = pt_ppns_.back() / 4;
    offset = pt_ppns_.back() % 4;
    pt_ppns_.pop_back();
  }
  else
  {
    physical_page_table_page = PageManager::instance()->allocPPN();
    offset = 0;
    for (size_t i = 1; i < 4; ++i)
      pt_ppns_.push_back(physical_page_table_page * 4 + i);
  }
  memset(((PageTableEntry *)getIdentAddressOfPPN(physical_page_table_page)) + offset * PAGE_TABLE_ENTRIES, 0, PT_SIZE);
  page_directory[pde_vpn].pt.pt_ppn = physical_page_table_page + PHYS_OFFSET_4K;
  page_directory[pde_vpn].pt.offset = offset;
  page_directory[pde_vpn].pt.size = PDE_SIZE_PT;
}

void ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size)
{
//  kprintfd("ArchMemory::mapPage: v: %x to p: %x\n",virtual_page,physical_page);
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;

  if (page_size == PAGE_SIZE)
  {
    if (page_directory[pde_vpn].pt.size == 0)
      insertPT(pde_vpn);

    PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
    pte_base[pte_vpn].bufferable = 0;
    pte_base[pte_vpn].cachable = 0;
    pte_base[pte_vpn].permissions = user_access ? 3 : 1;
    pte_base[pte_vpn].reserved = 0;
    pte_base[pte_vpn].page_ppn = physical_page + PHYS_OFFSET_4K;
    pte_base[pte_vpn].size = 2;
  }
  else
    assert(false); // currently only 4K pages for the userspace
}

// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT
// want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  debug(A_MEMORY, "ArchMemory::~ArchMemory(): Freeing page directory %x\n", page_dir_page_);
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  for (uint32 pde_vpn = 8; pde_vpn < PAGE_DIR_ENTRIES / 2; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pt.size == PDE_SIZE_PAGE)
    {
      assert(false); // currently not used and not implemented
    }
    else if (page_directory[pde_vpn].pt.size == PDE_SIZE_PT)
    {
      PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
      for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
      {
        if (pte_base[pte_vpn].size == 2)
        {
          pte_base[pte_vpn].size = 0;
          PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn - PHYS_OFFSET_4K);
        }
      }
      page_directory[pde_vpn].pt.size = PDE_SIZE_NONE;
      pt_ppns_.push_back((page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K) * 4 + page_directory[pde_vpn].pt.offset);
      for (size_t i = 0; i < 4; ++i)
        if (ustl::find(pt_ppns_.begin(), pt_ppns_.end(), page_directory[pde_vpn].pt.pt_ppn * 4 + i) == pt_ppns_.end())
          return;
      PageManager::instance()->freePPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K);
    }
  }
  PageManager::instance()->freePPN(page_dir_page_);
}

bool ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pt.size == PDE_SIZE_PAGE)
  {
    return true;
  }
  else if (page_directory[pde_vpn].pt.size == PDE_SIZE_PT)
  {
    PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
    if (pte_base[pte_vpn].size == 2)
    {
      return true;
    }
  }
  return false;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, uint32 *physical_page,
                                                   uint32 *physical_pte_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].page.size == PDE_SIZE_PAGE) // 1m page
  {
    if (physical_page)
      *physical_page = page_directory[pde_vpn].page.page_ppn - PHYS_OFFSET_1M;
    return 1024 * 1024;
  }
  else if (page_directory[pde_vpn].page.size == PDE_SIZE_PT) // 4k page
  {
    if (physical_pte_page)
      *physical_pte_page = page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K;
    PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
    if (pte_base[pte_vpn].size == 2)
    {
      if (physical_page)
        *physical_page = pte_base[pte_vpn].page_ppn - PHYS_OFFSET_4K;
      return PAGE_SIZE;
    }
    else
      return 0;
  }
  return 0;
}

void ArchMemory::mapKernelPage(uint32 virtual_page, uint32 physical_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  assert(page_directory[pde_vpn].page.size == PDE_SIZE_PT);
  PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
  assert(pte_base[pte_vpn].size == 0);
  pte_base[pte_vpn].permissions = 1;
  pte_base[pte_vpn].page_ppn = physical_page + PHYS_OFFSET_4K;
  pte_base[pte_vpn].size = 2;
}

void ArchMemory::unmapKernelPage(uint32 virtual_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  assert(page_directory[pde_vpn].page.size == PDE_SIZE_PT);
  PageTableEntry *pte_base = ((PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.pt_ppn - PHYS_OFFSET_4K)) + page_directory[pde_vpn].pt.offset * PAGE_TABLE_ENTRIES;
  assert(pte_base[pte_vpn].size == 2);
  pte_base[pte_vpn].size = 0;
  pte_base[pte_vpn].permissions = 0;
  PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn - PHYS_OFFSET_4K);
}

uint32 ArchMemory::getRootOfPagingStructure()
{
  return page_dir_page_;
}
