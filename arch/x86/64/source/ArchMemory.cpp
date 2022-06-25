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

PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(0x1000)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(0x1000)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));

ArchMemory kernel_arch_mem((size_t)ArchMemory::getKernelPagingStructureRootPhys()/PAGE_SIZE);

ArchMemory::ArchMemory()
{
  page_map_level_4_ = PageManager::instance()->allocPPN();
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

  PageManager::instance()->freePPN(m.page_ppn);
  if(pt_empty)   { PageManager::instance()->freePPN(m.pt_ppn);   }
  if(pd_empty)   { PageManager::instance()->freePPN(m.pd_ppn);   }
  if(pdpt_empty) { PageManager::instance()->freePPN(m.pdpt_ppn); }

  return true;
}

template<typename T>
void ArchMemory::insert(pointer map_ptr, uint64 index, uint64 ppn, uint64 bzero, uint64 size, uint64 user_access, uint64 writeable)
{
  assert(map_ptr & ~0xFFFFF00000000000ULL);
  T* map = (T*) map_ptr;
  debug(A_MEMORY, "%s: page %p index %llx ppn %llx user_access %llx size %llx\n",
        __PRETTY_FUNCTION__, map, index, ppn, user_access, size);
  if (bzero)
  {
    memset((void*) getIdentAddressOfPPN(ppn), 0, PAGE_SIZE);
    assert(((uint64* )map)[index] == 0);
  }
  map[index].size = size;
  map[index].writeable = writeable;
  map[index].page_ppn = ppn;
  map[index].user_access = user_access;
  map[index].present = 1;
}

bool ArchMemory::mapPage(vpn_t virtual_page, ppn_t physical_page, size_t user_access)
{
  debug(A_MEMORY, "%llx %zx %llx %zx\n", page_map_level_4_, virtual_page, physical_page, user_access);
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);
  assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

  if (m.pdpt == 0)
  {
    m.pdpt_ppn = PageManager::instance()->allocPPN();
    m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pdpt_ppn);
    insert<PageMapLevel4Entry>((pointer) m.pml4, m.pml4i, m.pdpt_ppn, 0, 0, 1, 1);
  }

  if (m.pd == 0)
  {
    m.pd_ppn = PageManager::instance()->allocPPN();
    m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);
    insert<PageDirPointerTablePageDirEntry>((pointer) m.pdpt, m.pdpti, m.pd_ppn, 0, 0, 1, 1);
  }

  if (m.pt == 0)
  {
    m.pt_ppn = PageManager::instance()->allocPPN();
    m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
    insert<PageDirPageTableEntry>((pointer) m.pd, m.pdi, m.pt_ppn, 0, 0, 1, 1);
  }

  if (m.page == 0)
  {
    insert<PageTableEntry>((pointer) m.pt, m.pti, physical_page, 0, 0, user_access, 1);
    return true;
  }

  return false;
}

ArchMemory::~ArchMemory()
{
  assert(currentThread->kernel_registers_->cr3 != page_map_level_4_ * PAGE_SIZE && "thread deletes its own arch memory");

  PageMapLevel4Entry* pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // free only lower half
  {
    if (pml4[pml4i].present)
    {
      PageDirPointerTableEntry* pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[pml4i].page_ppn);
      for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
      {
        if (pdpt[pdpti].pd.present)
        {
          assert(pdpt[pdpti].pd.size == 0);
          PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[pdpti].pd.page_ppn);
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (pd[pdi].pt.present)
            {
              assert(pd[pdi].pt.size == 0);
              PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
              for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
              {
                if (pt[pti].present)
                {
                  pt[pti].present = 0;
                  PageManager::instance()->freePPN(pt[pti].page_ppn);
                }
              }
              pd[pdi].pt.present = 0;
              PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);
            }
          }
          pdpt[pdpti].pd.present = 0;
          PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);
        }
      }
      pml4[pml4i].present = 0;
      PageManager::instance()->freePPN(pml4[pml4i].page_ppn);
    }
  }
  PageManager::instance()->freePPN(page_map_level_4_);
}

pointer ArchMemory::checkAddressValid(size_t vaddress_to_check)
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

const ArchMemoryMapping ArchMemory::resolveMapping(vpn_t vpage)
{
  return resolveMapping(page_map_level_4_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(ppn_t pml4, vpn_t vpage)
{
  ArchMemoryMapping m;

  m.pti = vpage;
  m.pdi = m.pti / PAGE_TABLE_ENTRIES;
  m.pdpti = m.pdi / PAGE_DIR_ENTRIES;
  m.pml4i = m.pdpti / PAGE_DIR_POINTER_TABLE_ENTRIES;

  m.pti %= PAGE_TABLE_ENTRIES;
  m.pdi %= PAGE_DIR_ENTRIES;
  m.pdpti %= PAGE_DIR_POINTER_TABLE_ENTRIES;
  m.pml4i %= PAGE_MAP_LEVEL_4_ENTRIES;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, vpn: %zx, pml4i: %llx(%llu), pdpti: %llx(%llu), pdi: %llx(%llu), pti: %llx(%llu)\n", vpage, m.pml4i, m.pml4i, m.pdpti, m.pdpti, m.pdi, m.pdi, m.pti, m.pti);
  }

  assert(pml4 < PageManager::instance()->getTotalNumPages());
  m.pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(pml4);
  m.pdpt = 0;
  m.pd = 0;
  m.pt = 0;
  m.page = 0;
  m.pml4_ppn = pml4;
  m.pdpt_ppn = 0;
  m.pd_ppn = 0;
  m.pt_ppn = 0;
  m.page_ppn = 0;
  m.page_size = 0;

  if (m.pml4[m.pml4i].present)
  {
    m.pdpt_ppn = m.pml4[m.pml4i].page_ppn;
    m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pml4[m.pml4i].page_ppn);
    if (m.pdpt[m.pdpti].pd.present && !m.pdpt[m.pdpti].pd.size) // 1gb page ?
    {
      m.pd_ppn = m.pdpt[m.pdpti].pd.page_ppn;
      if (m.pd_ppn > PageManager::instance()->getTotalNumPages())
      {
        debug(A_MEMORY, "%llx\n", m.pd_ppn);
      }
      assert(m.pd_ppn < PageManager::instance()->getTotalNumPages());
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pdpt[m.pdpti].pd.page_ppn);
      if (m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size) // 2mb page ?
      {
        m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
        assert(m.pt_ppn < PageManager::instance()->getTotalNumPages());
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
      //assert(m.page_ppn < PageManager::instance()->getTotalNumPages());
      m.page = getIdentAddressOfPPN(m.pdpt[m.pdpti].page.page_ppn);
    }
  }

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
      debug(A_MEMORY, "resolveMapping, vpn: %zx, pml4: %llx, pdpt[%s]: %llx, pd[%s]: %llx, pt[%s]: %llx, page[%s]: %llx\n",
            vpage,
            m.pml4_ppn,
            (m.pdpt ? "P" : "-"), m.pdpt_ppn,
            (m.pd ? "P" : "-"), m.pd_ppn,
            (m.pt ? "P" : "-"), m.pt_ppn,
            (m.page ? "P" : "-"), m.page_ppn);
  }
  return m;
}

size_t ArchMemory::get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, ppn_t *physical_page,
                                                   ppn_t *physical_pte_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), virtual_page);
  if (physical_page)
    *physical_page = m.page_ppn;
  if (physical_pte_page)
    *physical_pte_page = m.pt_ppn;
  return m.page_size;
}

bool ArchMemory::mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  //debug(A_MEMORY, "mapKernelPage, vpn: %zx, ppn: %zx\n", virtual_page, physical_page);
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getKernelPagingStructureRootVirt()) / PAGE_SIZE), virtual_page);

  if (m.page_size)
  {
      return false; // Page already mapped
  }

  m.pml4 = kernel_page_map_level_4;

  assert(m.pdpt || can_alloc_pages);
  if((!m.pdpt) && can_alloc_pages)
  {
          m.pdpt_ppn = PageManager::instance()->allocPPN();
          m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pdpt_ppn);
          insert<PageMapLevel4Entry>((pointer) m.pml4, m.pml4i, m.pdpt_ppn, 0, 0, 0, 1);
  }

  assert(m.pd || can_alloc_pages);
  if((!m.pd) && can_alloc_pages)
  {
          m.pd_ppn = PageManager::instance()->allocPPN();
          m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pd_ppn);
          insert<PageDirPointerTablePageDirEntry>((pointer) m.pdpt, m.pdpti, m.pd_ppn, 0, 0, 0, 1);
  }

  assert(m.pt || can_alloc_pages);
  if((!m.pt) && can_alloc_pages)
  {
          m.pt_ppn = PageManager::instance()->allocPPN();
          m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pt_ppn);
          insert<PageDirPageTableEntry>((pointer) m.pd, m.pdi, m.pt_ppn, 0, 0, 0, 1);
  }

  assert(!m.pt[m.pti].present);

  insert<PageTableEntry>((pointer)m.pt, m.pti, physical_page, 0, 0, 0, 1);
  if(memory_mapped_io)
  {
          m.pt[m.pti].write_through = 1;
          m.pt[m.pti].cache_disabled = 1;
  }

  //debug(A_MEMORY, "mapKernelPage, vpn: %zx, ppn: %zx end\n", virtual_page, physical_page);
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax"); // TODO: flushing caches after mapping a new page is pointless, remove

  return true;
}

void ArchMemory::unmapKernelPage(size_t virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), virtual_page);

  assert(m.page && (m.page_size == PAGE_SIZE));

  memset(&m.pt[m.pti], 0, sizeof(m.pt[m.pti]));
  if(free_page)
  {
    PageManager::instance()->freePPN(m.pt[m.pti].page_ppn);
  }
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

size_t ArchMemory::getPagingStructureRootPhys()
{
    return page_map_level_4_ * PAGE_SIZE;
}

PageMapLevel4Entry* ArchMemory::getKernelPagingStructureRootVirt()
{
    return kernel_page_map_level_4;
}

void ArchMemory::initKernelArchMem()
{
    new (&kernel_arch_mem) ArchMemory((size_t)getKernelPagingStructureRootPhys()/PAGE_SIZE);
}
