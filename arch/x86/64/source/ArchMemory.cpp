#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "kstring.h"
#include "ArchMulticore.h"
#include "ArchCommon.h"
#include "ArchThreads.h"
#include "Thread.h"

// Also see x86/common/source/ArchMemory.cpp for common functionality

PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

ArchMemory::ArchMemory()
{
  page_map_level_4_ = PageManager::instance().allocPPN();
  PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
  memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety
}

ArchMemory::ArchMemory(ppn_t pml4_ppn) :
    page_map_level_4_(pml4_ppn)
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

bool ArchMemory::unmapPage(vpn_t virtual_page)
{
  ArchMemoryMapping m = resolveMapping(virtual_page);
  debug(A_MEMORY, "Unmap %zx => pml4: %lx, pdpt: %lx, pd: %lx, pt: %lx, page: %lx\n",
        virtual_page, m.pml4_ppn, m.pdpt_ppn, m.pd_ppn, m.pt_ppn, m.page_ppn);
  assert(m.page && m.page_size == PAGE_SIZE);

  removeEntry(m.pt, m.pti);

  bool pdpt_empty = false;
  bool pd_empty = false;
  bool pt_empty = tableEmpty<PageTableEntry, PAGE_TABLE_ENTRIES>(m.pt);

  if (pt_empty)
  {
    removeEntry(&m.pd->pt, m.pdi);
    pd_empty = tableEmpty<PageDirPageTableEntry, PAGE_DIR_ENTRIES>(&m.pd->pt);
  }
  if (pd_empty)
  {
    removeEntry(&m.pdpt->pd, m.pdpti);
    pdpt_empty = tableEmpty<PageDirPointerTablePageDirEntry, PAGE_DIR_POINTER_TABLE_ENTRIES>(&m.pdpt->pd);
  }
  if (pdpt_empty)
  {
    removeEntry(m.pml4, m.pml4i);
  }

  flushAllTranslationCaches(virtual_page * PAGE_SIZE); // Needs to happen after page table entries have been modified but before PPNs are freed

  PageManager::instance().freePPN(m.page_ppn);
  if(pt_empty)   { PageManager::instance().freePPN(m.pt_ppn);   }
  if(pd_empty)   { PageManager::instance().freePPN(m.pd_ppn);   }
  if(pdpt_empty) { PageManager::instance().freePPN(m.pdpt_ppn); }

  return true;
}

template<typename T>
void ArchMemory::insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable, bool memory_mapped_io)
{
  debugAdvanced(A_MEMORY, "%s: page %p index %zx ppn %lx user_access %u\n",
    __PRETTY_FUNCTION__, table, index, ppn, user_access);

  assert((size_t)table & ~0xFFFFF00000000000ULL);
  assert(((uint64*)table)[index] == 0);

  if(memory_mapped_io)
  {
      table[index].write_through = 1;
      table[index].cache_disabled = 1;
  }

  table[index].size = 0;
  table[index].writeable = writeable;
  table[index].page_ppn = ppn;
  table[index].user_access = user_access;
  table[index].present = 1;
}

bool ArchMemory::mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access)
{
  ArchMemoryMapping m = resolveMapping(virtual_page);
  debug(A_MEMORY, "Map %zx => pml4: %lx, pdpt: %lx, pd: %lx, pt: %lx, page: %lx, user: %u\n",
        virtual_page, m.pml4_ppn, m.pdpt_ppn, m.pd_ppn, m.pt_ppn, physical_page, user_access);
  assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

  if (!m.pdpt)
  {
    m.pdpt_ppn = PageManager::instance().allocPPN();
    m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pdpt_ppn);
    insert(m.pml4, m.pml4i, m.pdpt_ppn, true, true, false);
  }

  if (!m.pd)
  {
    m.pd_ppn = PageManager::instance().allocPPN();
    m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);
    insert(&m.pdpt->pd, m.pdpti, m.pd_ppn, true, true, false);
  }

  if (!m.pt)
  {
    m.pt_ppn = PageManager::instance().allocPPN();
    m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
    insert(&m.pd->pt, m.pdi, m.pt_ppn, true, true, false);
  }

  if (!m.page)
  {
    insert(m.pt, m.pti, physical_page, user_access, true, false);
    return true;
  }

  return false;
}

ArchMemory::~ArchMemory()
{
    debug(A_MEMORY, "~ArchMemory(): Free PML4 %lx\n", page_map_level_4_);

    size_t cr3 = 0;
    asm("mov %%cr3, %[cr3]\n" : [cr3]"=g"(cr3));
    assert(cr3 != getValueForCR3() && "thread deletes its own arch memory");

    PageMapLevel4Entry* pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
    for (size_t pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // free only lower half
    {
        if (pml4[pml4i].present)
        {
            auto pdpt_ppn = pml4[pml4i].page_ppn;
            PageDirPointerTableEntry* pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pdpt_ppn);
            for (size_t pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
            {
                if (pdpt[pdpti].pd.present)
                {
                    assert(pdpt[pdpti].pd.size == 0);
                    auto pd_ppn = pdpt[pdpti].pd.page_ppn;
                    PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pd_ppn);
                    for (size_t pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
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
                    removeEntry(&pdpt->pd, pdpti);
                    PageManager::instance().freePPN(pd_ppn);
                }
            }
            removeEntry(pml4, pml4i);
            PageManager::instance().freePPN(pdpt_ppn);
        }
    }
    PageManager::instance().freePPN(page_map_level_4_);
}

pointer ArchMemory::checkAddressValid(size_t vaddress_to_check) const
{
  return checkAddressValid(page_map_level_4_, vaddress_to_check);
}

pointer ArchMemory::checkAddressValid(ppn_t pml4, size_t vaddress_to_check)
{
  ArchMemoryMapping m = resolveMapping(pml4, vaddress_to_check / PAGE_SIZE);
  if (m.page != 0)
  {
    debug(A_MEMORY, "checkAddressValid, pml4: %#zx, vaddr: %#zx -> true\n", (size_t)pml4, vaddress_to_check);
    return m.page | (vaddress_to_check % m.page_size);
  }
  else
  {
    debug(A_MEMORY, "checkAddressValid, pml4: %#zx, vaddr: %#zx -> false\n", (size_t)pml4, vaddress_to_check);
    return 0;
  }
}

const ArchMemoryMapping ArchMemory::resolveMapping(vpn_t vpage) const
{
  return resolveMapping(page_map_level_4_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(ppn_t pml4, vpn_t vpage)
{
  assert((vpage * PAGE_SIZE < USER_BREAK || vpage * PAGE_SIZE >= KERNEL_START) &&
         "This is not a valid vpn! Did you pass an address to resolveMapping?");
  ArchMemoryMapping m;

  m.pti   = vpage;
  m.pdi   = m.pti / PAGE_TABLE_ENTRIES;
  m.pdpti = m.pdi / PAGE_DIR_ENTRIES;
  m.pml4i = m.pdpti / PAGE_DIR_POINTER_TABLE_ENTRIES;

  m.pti   %= PAGE_TABLE_ENTRIES;
  m.pdi   %= PAGE_DIR_ENTRIES;
  m.pdpti %= PAGE_DIR_POINTER_TABLE_ENTRIES;
  m.pml4i %= PAGE_MAP_LEVEL_4_ENTRIES;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, vpn: %zx, pml4i: %lx(%lu), pdpti: %lx(%lu), pdi: %lx(%lu), pti: %lx(%lu)\n", vpage, m.pml4i, m.pml4i, m.pdpti, m.pdpti, m.pdi, m.pdi, m.pti, m.pti);
  }

  assert(pml4 < PageManager::instance().getTotalNumPages());
  m.pml4      = (PageMapLevel4Entry*) getIdentAddressOfPPN(pml4);
  m.pdpt      = 0;
  m.pd        = 0;
  m.pt        = 0;
  m.page      = 0;
  m.pml4_ppn  = pml4;
  m.pdpt_ppn  = 0;
  m.pd_ppn    = 0;
  m.pt_ppn    = 0;
  m.page_ppn  = 0;
  m.page_size = 0;

  if (m.pml4[m.pml4i].present)
  {
    m.pdpt_ppn = m.pml4[m.pml4i].page_ppn;
    m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pml4[m.pml4i].page_ppn);
    if (m.pdpt[m.pdpti].pd.present && !m.pdpt[m.pdpti].pd.size) // 1gb page ?
    {
      m.pd_ppn = m.pdpt[m.pdpti].pd.page_ppn;
      if (m.pd_ppn > PageManager::instance().getTotalNumPages())
      {
        debug(A_MEMORY, "%lx\n", m.pd_ppn);
      }
      assert(m.pd_ppn < PageManager::instance().getTotalNumPages());
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pdpt[m.pdpti].pd.page_ppn);
      if (m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size) // 2mb page ?
      {
        m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
        assert(m.pt_ppn < PageManager::instance().getTotalNumPages());
        m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pd[m.pdi].pt.page_ppn);
        if (m.pt[m.pti].present)
        {
          m.page = getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
          m.page_ppn = m.pt[m.pti].page_ppn;
          m.page_size = PAGE_SIZE;
        }
      }
      else if (m.pd[m.pdi].page.present)
      {
        m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES;
        m.page_ppn = m.pd[m.pdi].page.page_ppn;
        m.page = getIdentAddressOfPPN(m.pd[m.pdi].page.page_ppn);
      }
    }
    else if (m.pdpt[m.pdpti].page.present)
    {
      m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES * PAGE_DIR_ENTRIES;
      m.page_ppn = m.pdpt[m.pdpti].page.page_ppn;
      //assert(m.page_ppn < PageManager::instance().getTotalNumPages());
      m.page = getIdentAddressOfPPN(m.pdpt[m.pdpti].page.page_ppn);
    }
  }

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
      debug(A_MEMORY, "resolveMapping, vpn: %zx, pml4: %lx, pdpt[%s]: %lx, pd[%s]: %lx, pt[%s]: %lx, page[%s]: %lx\n",
            vpage,
            m.pml4_ppn,
            (m.pdpt ? "P" : "-"), m.pdpt_ppn,
            (m.pd   ? "P" : "-"), m.pd_ppn,
            (m.pt   ? "P" : "-"), m.pt_ppn,
            (m.page ? "P" : "-"), m.page_ppn);
  }
  return m;
}

size_t ArchMemory::get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, ppn_t *physical_page,
                                                   ppn_t *physical_pte_page)
{
  ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootPhys() / PAGE_SIZE, virtual_page);
  if (physical_page)
    *physical_page = m.page_ppn;
  if (physical_pte_page)
    *physical_pte_page = m.pt_ppn;
  return m.page_size;
}

bool ArchMemory::mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootPhys()/PAGE_SIZE, virtual_page);
  debug(A_MEMORY, "Map (kernel) %zx => pml4: %lx, pdpt: %lx, pd: %lx, pt: %lx, page: %lx\n",
        virtual_page, m.pml4_ppn, m.pdpt_ppn, m.pd_ppn, m.pt_ppn, physical_page);

  assert(m.pdpt || can_alloc_pages);
  if(!m.pdpt && can_alloc_pages)
  {
          m.pdpt_ppn = PageManager::instance().allocPPN();
          m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pdpt_ppn);
          insert(m.pml4, m.pml4i, m.pdpt_ppn, false, true, false);
  }

  assert(m.pd || can_alloc_pages);
  if(!m.pd && can_alloc_pages)
  {
          m.pd_ppn = PageManager::instance().allocPPN();
          m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);
          insert(&m.pdpt->pd, m.pdpti, m.pd_ppn, false, true, false);
  }

  assert(m.pt || can_alloc_pages);
  if(!m.pt && can_alloc_pages)
  {
          m.pt_ppn = PageManager::instance().allocPPN();
          m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
          insert(&m.pd->pt, m.pdi, m.pt_ppn, false, true, false);
  }

  if (!m.page)
  {
      insert(m.pt, m.pti, physical_page, false, true, memory_mapped_io);
      return true;
  }

  return false; // already mapped
}

void ArchMemory::unmapKernelPage(size_t virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootPhys() / PAGE_SIZE, virtual_page);

  assert(m.page && (m.page_size == PAGE_SIZE));

  memset(&m.pt[m.pti], 0, sizeof(m.pt[m.pti]));

  flushAllTranslationCaches(virtual_page * PAGE_SIZE); // Needs to happen after page table entries have been modified but before PPNs are freed

  if(free_page)
  {
    PageManager::instance().freePPN(m.pt[m.pti].page_ppn);
  }
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

size_t ArchMemory::getPagingStructureRootPhys() const
{
    return page_map_level_4_ * PAGE_SIZE;
}

PageMapLevel4Entry* ArchMemory::getKernelPagingStructureRootVirt()
{
    return kernel_page_map_level_4;
}

ArchMemory& ArchMemory::kernelArchMemory()
{
    static ArchMemory kernel_arch_mem((size_t)ArchMemory::getKernelPagingStructureRootPhys()/PAGE_SIZE);
    return kernel_arch_mem;
}
