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
#include "paging-definitions.h"


// Also see x86/common/source/ArchMemory.cpp for common functionality

PageDirPointerTableEntry kernel_page_directory_pointer_table[PAGE_DIRECTORY_POINTER_TABLE_ENTRIES] __attribute__((aligned(0x20)));
PageDirEntry kernel_page_directory[4 * PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_tables[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));

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
  debug(A_MEMORY, "Unmap %zx => pdpt: %x, pd: %x, pt: %x, page: %x\n",
        virtual_page, getPagingStructureRootPhys(), m.pd_ppn, m.pt_ppn, m.page_ppn);
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

  PageManager::instance().freePPN(m.page_ppn);
  if(pt_empty) { PageManager::instance().freePPN(m.pt_ppn); }
  if(pd_empty) { PageManager::instance().freePPN(m.pd_ppn); }
}

template<typename T>
void ArchMemory::insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable)
{
    if (A_MEMORY & OUTPUT_ADVANCED)
        debug(A_MEMORY, "%s: page %p index %zx ppn %x user_access %u, writeable %u\n",
              __PRETTY_FUNCTION__, table, index, ppn, user_access, writeable);

    assert(((uint64*)table)[index] == 0);

    // conditional compilation at compile time!
    if constexpr (T::supports_writeable::value)
    {
        table[index].writeable = writeable;
    }
    if constexpr (T::supports_user_access::value)
    {
        table[index].user_access = user_access;
    }

    table[index].page_ppn = ppn;
    table[index].present = 1;
}

bool ArchMemory::mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access)
{
    ArchMemoryMapping m = resolveMapping(virtual_page);
    debug(A_MEMORY, "Map %zx => pdpt: %x pd: %x, pt: %x, page: %x, user: %u\n",
          virtual_page, getPagingStructureRootPhys(), m.pd_ppn, m.pt_ppn, physical_page, user_access);
    assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

    if (m.pd == 0)
    {
        m.pd_ppn = PageManager::instance().allocPPN();
        m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);
        insert(m.pdpt, m.pdpti, m.pd_ppn, user_access, true);
    }

    if (m.pt == 0)
    {
        m.pt_ppn = PageManager::instance().allocPPN();
        m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
        insert((PageDirPageTableEntry*)m.pd, m.pdi, m.pt_ppn, user_access, true);
    }

    if (m.page == 0)
    {
        insert(m.pt, m.pti, physical_page, user_access, true);
        return true;
    }

    return false;
}

ArchMemory::~ArchMemory()
{
    debug(A_MEMORY, "~ArchMemory(): Free PDPT %x\n", getPagingStructureRootPhys());

    size_t cr3 = 0;
    asm("mov %%cr3, %[cr3]\n" : [cr3]"=g"(cr3));
    assert(cr3 != getValueForCR3() && "thread deletes its own arch memory");

    PageDirPointerTableEntry* pdpt = page_dir_pointer_table_;
    for (size_t pdpti = 0; pdpti < PAGE_DIRECTORY_POINTER_TABLE_ENTRIES/2; pdpti++)
    {
        if (pdpt[pdpti].present)
        {
            auto pd_ppn = pdpt[pdpti].page_ppn;
            PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pd_ppn);
            for (size_t pdi = 0; pdi < PAGE_DIRECTORY_ENTRIES; pdi++)
            {
                if (pd[pdi].pt.present)
                {
                    assert(pd[pdi].pt.size == 0);
                    auto pt_ppn = pd[pdi].pt.page_ppn;
                    PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pt_ppn);
                    for (size_t pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
                    {
                        if (pt[pti].present)
                        {
                            auto page_ppn = pt[pti].page_ppn;
                            removeEntry(pt, pti);
                            PageManager::instance().freePPN(page_ppn);
                        }
                    }
                    removeEntry(&pd->pt, pdi);
                    PageManager::instance().freePPN(pt_ppn);
                }
            }
            removeEntry(pdpt, pdpti);
            PageManager::instance().freePPN(pd_ppn);
        }
    }
}

pointer ArchMemory::checkAddressValid(uint32 vaddress_to_check) const
{
    return checkAddressValid(page_dir_pointer_table_, vaddress_to_check);
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

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage) const
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
      m.pd_ppn = m.pdpt[m.pdpti].page_ppn;
      assert(m.pd_ppn < PageManager::instance().getTotalNumPages());
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);

      if(m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size)
      {
          m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
          assert(m.pt_ppn < PageManager::instance().getTotalNumPages());
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
          *physical_pte_page = page_directory[pde_vpn].pt.page_ppn;
        PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_ppn);
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
      m.pd_ppn = PageManager::instance().allocPPN();
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);

      m.pdpt[m.pdpti].page_ppn = m.pd_ppn;
      m.pdpt[m.pdpti].present = 1;
  }

  assert(m.pt || can_alloc_pages);
  if((!m.pt) && can_alloc_pages)
  {
      m.pt_ppn = PageManager::instance().allocPPN();
      m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);

      m.pd[m.pdi].pt.page_ppn = m.pt_ppn;
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
      PageManager::instance().freePPN(m.page_ppn);
  }

  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

size_t ArchMemory::getPagingStructureRootPhys() const
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

ArchMemory& ArchMemory::kernelArchMemory()
{
    static ArchMemory kernel_arch_mem(getKernelPagingStructureRootVirt());
    return kernel_arch_mem;
}
