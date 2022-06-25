#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "assert.h"
#include "offsets.h"
#include "PageManager.h"
#include "kstring.h"
#include <new>

// Also see x86/common/source/ArchMemory.cpp for common functionality

PageDirPointerTableEntry kernel_page_directory_pointer_table[PAGE_DIRECTORY_POINTER_TABLE_ENTRIES] __attribute__((aligned(0x20)));
PageDirEntry kernel_page_directory[4 * PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_tables[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));

ArchMemory kernel_arch_mem(ArchMemory::getKernelPagingStructureRootVirt());

ArchMemory::ArchMemory() : page_dir_pointer_table_((PageDirPointerTableEntry*) (((uint32) page_dir_pointer_table_space_ + 0x20) & (~0x1F)))
{
  memcpy(page_dir_pointer_table_, kernel_page_directory_pointer_table, sizeof(PageDirPointerTableEntry) * PAGE_DIRECTORY_POINTER_TABLE_ENTRIES);
  memset(page_dir_pointer_table_, 0, sizeof(PageDirPointerTableEntry) * PAGE_DIRECTORY_POINTER_TABLE_ENTRIES/2); // should be zero, this is just for safety
}

ArchMemory::ArchMemory(PageDirPointerTableEntry* pdpt) :
    page_dir_pointer_table_(pdpt)
{
}

template<typename T, size_t NUM_ENTRIES>
bool ArchMemory::tableEmpty(T* table)
{
    for (size_t i = 0; i < NUM_ENTRIES; i++)
    {
        if (table[i].present)
            return false;
    }
    return true;
}

template<typename T>
void ArchMemory::removeEntry(T* table, size_t index)
{
    assert(table[index].present);

    table[index].present = 0;
    memset(&table[index], 0, sizeof(table[index]));
}

void ArchMemory::unmapPage(uint32 virtual_page)
{
  ArchMemoryMapping m = resolveMapping(virtual_page);
  assert(m.page != 0 && m.page_size == PAGE_SIZE);

  removeEntry(m.pt, m.pti);

  bool pd_empty = false;
  bool pt_empty = tableEmpty<PageTableEntry, PAGE_TABLE_ENTRIES>(m.pt);

  if (pt_empty)
  {
      removeEntry(&m.pd->pt, m.pdi);
      pd_empty = tableEmpty<PageDirPageTableEntry, PAGE_DIRECTORY_ENTRIES>(&m.pd->pt);
  }
  if (pd_empty)
  {
      removeEntry(m.pdpt, m.pdpti);
  }

  flushAllTranslationCaches(virtual_page * PAGE_SIZE); // Needs to happen after page table entries have been modified but before PPNs are freed

  PageManager::instance()->freePPN(m.page_ppn);
  if(pt_empty) { PageManager::instance()->freePPN(m.pt_ppn); }
  if(pd_empty) { PageManager::instance()->freePPN(m.pd_ppn); }
}

void ArchMemory::insertPD(uint32 pdpt_vpn, uint32 physical_page_directory_page)
{
  debug(A_MEMORY, "insertPD: pdpt %p pdpt_vpn %x physical_page_table_page %x\n",
        page_dir_pointer_table_,pdpt_vpn,physical_page_directory_page);
  memset((void*)getIdentAddressOfPPN(physical_page_directory_page), 0,PAGE_SIZE);
  memset((void*)(page_dir_pointer_table_ + pdpt_vpn), 0, sizeof(PageDirPointerTableEntry));
  page_dir_pointer_table_[pdpt_vpn].page_directory_ppn = physical_page_directory_page;
  page_dir_pointer_table_[pdpt_vpn].present = 1;
}

void ArchMemory::insertPT(PageDirEntry* page_directory, uint32 pde_vpn, uint32 physical_page_table_page)
{
  debug(A_MEMORY, "insertPT: page_directory %p pde_vpn %x physical_page_table_page %x\n",
        page_directory,pde_vpn,physical_page_table_page);
  memset((void*)getIdentAddressOfPPN(physical_page_table_page), 0, PAGE_SIZE);
  memset((void*)(page_directory + pde_vpn), 0, sizeof(PageDirPointerTableEntry));
  page_directory[pde_vpn].pt.writeable = 1;
  page_directory[pde_vpn].pt.size = 0;
  page_directory[pde_vpn].pt.page_table_ppn = physical_page_table_page;
  page_directory[pde_vpn].pt.user_access = 1;
  page_directory[pde_vpn].pt.present = 1;
}

bool ArchMemory::mapPage(uint32 virtual_page, uint32 physical_page, uint32 user_access)
{
  RESOLVEMAPPING(page_dir_pointer_table_,virtual_page);

  if (page_dir_pointer_table_[pdpte_vpn].present == 0)
  {
    uint32 pd_ppn = PageManager::instance()->allocPPN();
    page_directory = (PageDirEntry*) getIdentAddressOfPPN(pd_ppn);
    insertPD(pdpte_vpn, pd_ppn);
  }

  if (page_directory[pde_vpn].pt.present == 0)
  {
    insertPT(page_directory, pde_vpn, PageManager::instance()->allocPPN());
  }
  assert(page_directory[pde_vpn].page.size == 0);

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

ArchMemory::~ArchMemory()
{
  debug ( A_MEMORY,"ArchMemory::~ArchMemory(): Freeing page dir pointer table %p\n",page_dir_pointer_table_ );
  if(page_dir_pointer_table_[0].present)
    freePageDirectory(page_dir_pointer_table_[0].page_directory_ppn); // 0-1 GiB
  if(page_dir_pointer_table_[1].present)
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
            PageManager::instance()->freePPN(page_directory[pde_vpn].page.page_ppn*1024 + p);
      }
      else
      {
        PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
        for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
        {
          if (pte_base[pte_vpn].present)
          {
            pte_base[pte_vpn].present = 0;
            PageManager::instance()->freePPN(pte_base[pte_vpn].page_ppn);
          }
        }
        page_directory[pde_vpn].pt.present=0;
        PageManager::instance()->freePPN(page_directory[pde_vpn].pt.page_table_ppn);
      }
    }
  }
  PageManager::instance()->freePPN(physical_page_directory_page);
}

pointer ArchMemory::checkAddressValid(uint32 vaddress_to_check)
{
  RESOLVEMAPPING(page_dir_pointer_table_, vaddress_to_check / PAGE_SIZE);
  if (page_dir_pointer_table_[pdpte_vpn].present)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      if (page_directory[pde_vpn].page.size)
        return getIdentAddressOfPPN(page_directory[pde_vpn].page.page_ppn,PAGE_SIZE * PAGE_TABLE_ENTRIES) | (vaddress_to_check % (PAGE_SIZE * PAGE_TABLE_ENTRIES));

      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_table_ppn);
      if (pte_base[pte_vpn].present)
        return getIdentAddressOfPPN(pte_base[pte_vpn].page_ppn) | (vaddress_to_check % PAGE_SIZE);
    }
  }
  return 0;
}

pointer ArchMemory::checkAddressValid(PageDirPointerTableEntry* pdpt, uint32 vaddress_to_check)
{
    ArchMemoryMapping m = resolveMapping(pdpt, vaddress_to_check / PAGE_SIZE);
    if (m.page != 0)
    {
        debug(A_MEMORY, "checkAddressValid, pdpt: %p, vaddr: %#zx -> true\n", pdpt, vaddress_to_check);
        return m.page | (vaddress_to_check % m.page_size);
    }
    else
    {
        debug(A_MEMORY, "checkAddressValid, pdpt: %p, vaddr: %#zx -> false\n", pdpt, vaddress_to_check);
        return 0;
    }
}

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage)
{
    return resolveMapping(page_dir_pointer_table_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(PageDirPointerTableEntry* pdpt, vpn_t vpage)
{
  ArchMemoryMapping m;

  VAddr a{vpage*PAGE_SIZE};

  m.pti = a.pti;
  m.pdi = a.pdi;
  m.pdpti = a.pdpti;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, vpn: %#zx, pdpti: %zx(%zu), pdi: %zx(%zu), pti: %zx(%zu)\n", vpage, m.pdpti, m.pdpti, m.pdi, m.pdi, m.pti, m.pti);
  }

  m.pdpt = pdpt;
  m.pd = 0;
  m.pt = 0;
  m.page = 0;
  m.pd_ppn = 0;
  m.pt_ppn = 0;
  m.page_ppn = 0;
  m.page_size = 0;

  if(m.pdpt[m.pdpti].present)
  {
      m.pd_ppn = m.pdpt[m.pdpti].page_directory_ppn;
      assert(m.pd_ppn < PageManager::instance()->getTotalNumPages());
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);

      if(m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size)
      {
          m.pt_ppn = m.pd[m.pdi].pt.page_table_ppn;
          assert(m.pt_ppn < PageManager::instance()->getTotalNumPages());
          m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
          if(m.pt[m.pti].present)
          {
              m.page_size = PAGE_SIZE;
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
  }

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
      debug(A_MEMORY, "resolveMapping, vpn: %#zx, pdpt[%s]: %p, pd[%s]: %#zx, pt[%s]: %#zx, page[%s]: %#zx, size: %#zx\n",
            vpage,
            (m.pdpt ? "P" : "-"), m.pdpt,
            (m.pd ? "P" : "-"), m.pd_ppn,
            (m.pt ? "P" : "-"), m.pt_ppn,
            (m.page ? "P" : "-"), m.page_ppn,
            m.page_size);
  }

  return m;
}

uint32 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(uint32 virtual_page, size_t *physical_page, uint32 *physical_pte_page)
{
  PageDirPointerTableEntry *pdpt = kernel_page_directory_pointer_table;
  RESOLVEMAPPING(pdpt, virtual_page);
  if (pdpt[pdpte_vpn].present)
  {
    if (page_directory[pde_vpn].pt.present)
    {
      if (page_directory[pde_vpn].page.size)
      {
        if (physical_page)
          *physical_page = page_directory[pde_vpn].page.page_ppn;
        return (PAGE_SIZE*PAGE_TABLE_ENTRIES);
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
      }
    }
  }
  return 0;
}

bool ArchMemory::mapKernelPage(uint32 virtual_page, uint32 physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  debug(A_MEMORY, "Map kernel page %#zx -> PPN %#zx, alloc new pages: %u, mmio: %u\n", virtual_page, physical_page, can_alloc_pages, memory_mapped_io);

  ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootVirt(), virtual_page);

  if (m.page_size)
  {
      return false; // Page already mapped
  }

  assert(m.pd || can_alloc_pages);
  if((!m.pd) && can_alloc_pages)
  {
      m.pd_ppn = PageManager::instance()->allocPPN();
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);

      m.pdpt[m.pdpti].page_directory_ppn = m.pd_ppn;
      m.pdpt[m.pdpti].present = 1;
  }

  assert(m.pt || can_alloc_pages);
  if((!m.pt) && can_alloc_pages)
  {
      m.pt_ppn = PageManager::instance()->allocPPN();
      m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);

      m.pd[m.pdi].pt.page_table_ppn = m.pt_ppn;
      m.pd[m.pdi].pt.writeable = 1;
      m.pd[m.pdi].pt.present = 1;
  }

  assert(!m.pt[m.pti].present);

  if(memory_mapped_io)
  {
      m.pt[m.pti].write_through = 1;
      m.pt[m.pti].cache_disabled = 1;
  }

  m.pt[m.pti].page_ppn = physical_page;
  m.pt[m.pti].writeable = 1;
  m.pt[m.pti].present = 1;

  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");

  return true;
}

void ArchMemory::unmapKernelPage(uint32 virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootVirt(), virtual_page);
  assert(m.page && (m.page_size == PAGE_SIZE));

  memset(&m.pt[m.pti], 0, sizeof(m.pt[m.pti]));

  if(free_page)
  {
      PageManager::instance()->freePPN(m.page_ppn);
  }

  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

size_t ArchMemory::getPagingStructureRootPhys()
{
    // last 5 bits must be zero!
    assert(((uint32)page_dir_pointer_table_ & 0x1F) == 0);
    size_t ppn = 0;
    if (get_PPN_Of_VPN_In_KernelMapping(((size_t)page_dir_pointer_table_) / PAGE_SIZE, &ppn) > 0)
        return ppn * PAGE_SIZE + ((size_t)page_dir_pointer_table_ % PAGE_SIZE);
    assert(false);
    return 0;
}

PageDirPointerTableEntry* ArchMemory::getKernelPagingStructureRootVirt()
{
  return kernel_page_directory_pointer_table;
}

void ArchMemory::initKernelArchMem()
{
    new (&kernel_arch_mem) ArchMemory(getKernelPagingStructureRootVirt());
}
