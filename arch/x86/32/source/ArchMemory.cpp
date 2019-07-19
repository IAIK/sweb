#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "kstring.h"
#include "offsets.h"

PageDirEntry kernel_page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_tables[4 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance()->allocPPN();
  PageDirEntry *new_page_directory = (PageDirEntry*) getIdentAddressOfPPN(page_dir_page_);
  memcpy(new_page_directory, kernel_page_directory, PAGE_SIZE);
  memset(new_page_directory, 0, PAGE_SIZE / 2); // should be zero, this is just for safety
}

// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT want to free Kernel Pages
ArchMemory::~ArchMemory()
{
  debug(A_MEMORY, "ArchMemory::~ArchMemory(): Freeing page directory %x\n", page_dir_page_);
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  for (uint32 pde_vpn = 0; pde_vpn < PAGE_TABLE_ENTRIES / 2; ++pde_vpn)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      assert(!page_directory[pde_vpn].page.size); // only 4 KiB pages allowed
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      for (uint32 pte_vpn = 0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
      {
        if (pte_base[pte_vpn].present)
        {
          unmapPage(pde_vpn * PAGE_TABLE_ENTRIES + pte_vpn);
          if(!page_directory[pde_vpn].pt.present)
        	  break;
        }
      }
    }
  }
  PageManager::instance()->freePPN(page_dir_page_);
}

void ArchMemory::checkAndRemovePT(uint32 pde_vpn)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  assert(page_directory[pde_vpn].page.size == 0);

  assert(page_directory[pde_vpn].pt.present);
  assert(!page_directory[pde_vpn].page.size);

  for (uint32 pte_vpn = 0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
    if (pte_base[pte_vpn].present > 0)
      return; //not empty -> do nothing

  //else:
  page_directory[pde_vpn].pt.present = 0;
  PageManager::instance()->freePPN(page_directory[pde_vpn].pt.page_table_ppn);
  ((uint32*)page_directory)[pde_vpn] = 0; // for easier debugging
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  RESOLVEMAPPING(page_dir_page_, virtual_page);

  assert(page_directory[pde_vpn].pt.present);
  assert(!page_directory[pde_vpn].page.size); // only 4 KiB pages allowed

  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  assert(pte_base[pte_vpn].present);

  pte_base[pte_vpn].present = 0;
  PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn);
  ((uint32*)pte_base)[pte_vpn] = 0; // for easier debugging

  checkAndRemovePT(pde_vpn);
}

void ArchMemory::insertPT(uint32 pde_vpn, uint32 physical_page_table_page)
{
  PageDirEntry *page_directory = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page_);
  assert(!page_directory[pde_vpn].pt.present);
  memset((void*) getIdentAddressOfPPN(physical_page_table_page), 0, PAGE_SIZE);
  page_directory[pde_vpn].pt.writeable = 1;
  page_directory[pde_vpn].pt.size = 0;
  page_directory[pde_vpn].pt.page_table_ppn = physical_page_table_page;
  page_directory[pde_vpn].pt.user_access = 1;
  page_directory[pde_vpn].pt.present = 1;
}

bool ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access)
{
  RESOLVEMAPPING(page_dir_page_, virtual_page);

  if(page_directory[pde_vpn].pt.present == 0)
  {
    insertPT(pde_vpn, PageManager::instance()->allocPPN());
  }
  assert(!page_directory[pde_vpn].page.size);


  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  if(pte_base[pte_vpn].present == 0)
  {
    pte_base[pte_vpn].writeable = 1;
    pte_base[pte_vpn].user_access = user_access;
    pte_base[pte_vpn].page_ppn = physical_page;
    pte_base[pte_vpn].present = 1;
    return true;
  }

  return false;
}

pointer ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  uint32 virtual_page = vaddress_to_check / PAGE_SIZE;
  RESOLVEMAPPING(page_dir_page_, virtual_page);
  if (page_directory[pde_vpn].pt.present)
  {
    if (page_directory[pde_vpn].page.size)
      return getIdentAddressOfPPN(page_directory[pde_vpn].page.page_ppn,PAGE_SIZE * PAGE_TABLE_ENTRIES) | (vaddress_to_check % (PAGE_SIZE * PAGE_TABLE_ENTRIES));

    PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
    if (pte_base[pte_vpn].present)
      return getIdentAddressOfPPN(pte_base[pte_vpn].page_ppn) | (vaddress_to_check % PAGE_SIZE);
  }
  return 0;
}

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage)
{
  return resolveMapping(page_dir_page_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(ppn_t pd, size_t vpage)
{
  ArchMemoryMapping m;

  VAddr a{vpage*PAGE_SIZE};

  m.pti = a.pti;
  m.pdi = a.pdi;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, pd: %zx, vpn: %zx, pdi: %zx(%zu), pti: %zx(%zu)\n", pd, vpage, m.pdi, m.pdi, m.pti, m.pti);
  }

  assert(pd < PageManager::instance()->getTotalNumPages());
  m.pd = (PageDirEntry*) getIdentAddressOfPPN(pd);
  m.pt = 0;
  m.page = 0;
  m.pd_ppn = pd;
  m.pt_ppn = 0;
  m.page_ppn = 0;
  m.page_size = 0;

  if(m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size)
  {
          m.pt_ppn = m.pd[m.pdi].pt.page_table_ppn;
          assert(m.pt_ppn < PageManager::instance()->getTotalNumPages());
          m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
          if(m.pt[m.pti].present)
          {
                  m.page_ppn = m.pt[m.pti].page_ppn;
                  m.page = getIdentAddressOfPPN(m.page_ppn);
          }
  }
  else if(m.pd[m.pdi].pt.present && m.pd[m.pdi].pt.size)
  {
          m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES;
          m.page_ppn = m.pd[m.pdi].page.page_ppn;
          m.page = getIdentAddressOfPPN(m.page_ppn);
  }

  debug(A_MEMORY, "VPN %#zx: %#zx -> %#zx -> %#zx\n", vpage, m.pd_ppn, m.pt_ppn, m.page_ppn);

  return m;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, uint32 *physical_page,
                                                   uint32 *physical_pte_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  if (page_directory[pde_vpn].pt.present) //the present bit is the same for 4k and 4m
  {
    if (page_directory[pde_vpn].page.size)
    {
      if (physical_page)
        *physical_page = page_directory[pde_vpn].page.page_ppn;
      return (PAGE_SIZE * 1024U);
    }
    else
    {
      if (physical_pte_page)
        *physical_pte_page = page_directory[pde_vpn].pt.page_table_ppn;
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      if (pte_base[pte_vpn].present)
      {
        if (physical_page)
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

void ArchMemory::mapKernelPage(uint32 virtual_page, uint32 physical_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  assert(page_directory[pde_vpn].pt.present && page_directory[pde_vpn].pt.size == 0);
  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  assert(!pte_base[pte_vpn].present);
  pte_base[pte_vpn].writeable = 1;
  pte_base[pte_vpn].page_ppn = physical_page;
  pte_base[pte_vpn].present = 1;
  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

void ArchMemory::unmapKernelPage(uint32 virtual_page)
{
  PageDirEntry *page_directory = kernel_page_directory;
  uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
  uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
  assert(page_directory[pde_vpn].pt.present && page_directory[pde_vpn].pt.size == 0);
  PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
  assert(pte_base[pte_vpn].present);
  pte_base[pte_vpn].present = 0;
  pte_base[pte_vpn].writeable = 0;
  PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn);
  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

uint32 ArchMemory::getRootOfPagingStructure()
{
  return page_dir_page_;
}

PageDirEntry* ArchMemory::getRootOfKernelPagingStructure()
{
  return kernel_page_directory;
}

void ArchMemory::loadPagingStructureRoot(size_t cr3_value)
{
        __asm__ __volatile__("movl %[cr3_value], %%cr3\n"
                             ::[cr3_value]"r"(cr3_value));
}

uint32 ArchMemory::getValueForCR3()
{
  return getRootOfPagingStructure() * PAGE_SIZE;
}

pointer ArchMemory::getIdentAddressOfPPN(uint32 ppn, uint32 page_size /* optional */)
{
  return (3U * 1024U * 1024U * 1024U) + (ppn * page_size);
}

pointer ArchMemory::getIdentAddress(size_t address)
{
  return (3U * 1024U * 1024U * 1024U) | (address);
}



