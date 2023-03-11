#include "ArchMemory.h"

#include "PageManager.h"
#include "kprintf.h"
#include "kstring.h"
#include "offsets.h"

#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"

#include "assert.h"

// Also see x86/common/source/ArchMemory.cpp for common functionality

PageDirEntry kernel_page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_tables[4 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));

ArchMemory::ArchMemory()
{
  page_dir_page_ = PageManager::instance().allocPPN();
  PageDirEntry *new_page_directory = (PageDirEntry*) getIdentAddressOfPPN(page_dir_page_);
  memcpy(new_page_directory, kernel_page_directory, PAGE_SIZE);
  memset(new_page_directory, 0, PAGE_SIZE / 2); // should be zero, this is just for safety
}

ArchMemory::ArchMemory(size_t page_dir_ppn) :
    page_dir_page_(page_dir_ppn)
{
}

// only free pte's < PAGE_TABLE_ENTRIES/2 because we do NOT want to free Kernel Pages
ArchMemory::~ArchMemory()
{
    debug(A_MEMORY, "~ArchMemory(): Free PD %x\n", page_dir_page_);

    size_t cr3 = 0;
    asm("mov %%cr3, %[cr3]\n" : [cr3]"=g"(cr3));
    assert(cr3 != getValueForCR3() && "thread deletes its own arch memory");

    PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(page_dir_page_);
    for (size_t pdi = 0; pdi < PAGE_TABLE_ENTRIES/2; ++pdi)
    {
        if (pd[pdi].pt.present)
        {
            assert(!pd[pdi].page.size); // only 4 KiB pages allowed
            auto pt_ppn = pd[pdi].pt.page_ppn;
            PageTableEntry *pt = (PageTableEntry *) getIdentAddressOfPPN(pt_ppn);
            for (size_t pti = 0; pti < PAGE_TABLE_ENTRIES; ++pti)
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
    PageManager::instance().freePPN(page_dir_page_);
}


void ArchMemory::printMappedPages()
{
    printMappedPages(page_dir_page_);
}

void ArchMemory::printMappedPages(uint32 page_dir_page)
{
    debug(A_MEMORY, "ArchMemory::print mapped pages for PD %x\n", page_dir_page);


    PageDirEntry *pd = (PageDirEntry *) getIdentAddressOfPPN(page_dir_page);
    for (uint32 pdi = 0; pdi < PAGE_TABLE_ENTRIES; ++pdi)
    {
        if (pd[pdi].pt.present)
        {
            if(!pd[pdi].pt.size)
            {
                PageTableEntry *pt = (PageTableEntry *) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
                for (uint32 pti = 0; pti < PAGE_TABLE_ENTRIES; ++pti)
                {
                    if (pt[pti].present)
                    {
                        VAddr a{};
                        a.pdi = pdi;
                        a.pti = pti;
                        debug(A_MEMORY, "[%zx - %zx] -> {%zx - %zx} (%u, %u) U: %u, W: %u, T: %u, C: %u\n",
                              a.addr, a.addr + PAGE_SIZE,
                              pt[pti].page_ppn*PAGE_SIZE, pt[pti].page_ppn*PAGE_SIZE + PAGE_SIZE,
                              pdi, pti,
                              pt[pti].user_access, pt[pti].writeable, pt[pti].write_through, pt[pti].cache_disabled);
                    }
                }
            }
            else
            {
                VAddr a{};
                a.pdi = pdi;
                debug(A_MEMORY, "[%zx - %zx] -> {%zx - %zx} (%u) LARGE PAGE U: %u, W: %u, T: %u, C: %u\n",
                      a.addr, a.addr + PAGE_SIZE*PAGE_TABLE_ENTRIES,
                      pd[pdi].page.page_ppn*PAGE_SIZE, pd[pdi].page.page_ppn*PAGE_SIZE + PAGE_SIZE*PAGE_TABLE_ENTRIES,
                      pdi,
                      pd[pdi].page.user_access, pd[pdi].page.writeable, pd[pdi].page.write_through, pd[pdi].page.cache_disabled);
            }
        }
    }
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
  debug(A_MEMORY, "Unmap %zx => pd: %x, pt: %x, page: %x\n",
        virtual_page, m.pd_ppn, m.pt_ppn, m.page_ppn);
  assert(m.page && m.page_size == PAGE_SIZE);

  removeEntry(m.pt, m.pti);

  bool pt_empty = tableEmpty<PageTableEntry, PAGE_TABLE_ENTRIES>(m.pt);
  if (pt_empty)
  {
      removeEntry((PageDirPageTableEntry*)m.pd, m.pdi);
  }

  flushAllTranslationCaches(virtual_page * PAGE_SIZE); // Needs to happen after page table entries have been modified but before PPNs are freed

  PageManager::instance().freePPN(m.page_ppn);
  if(pt_empty) { PageManager::instance().freePPN(m.pt_ppn); }
}

template<typename T>
void ArchMemory::insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable)
{
    if (A_MEMORY & OUTPUT_ADVANCED)
        debug(A_MEMORY, "%s: page %p index %zx ppn %x user_access %u, writeable %u\n",
              __PRETTY_FUNCTION__, table, index, ppn, user_access, writeable);

    assert(((uint32*)table)[index] == 0);

    table[index].writeable = writeable;
    table[index].user_access = user_access;
    table[index].page_ppn = ppn;
    table[index].present = 1;
}

bool ArchMemory::mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access)
{
    ArchMemoryMapping m = resolveMapping(virtual_page);
    debug(A_MEMORY, "Map %zx => pd: %x, pt: %x, page: %x, user: %u\n",
          virtual_page, m.pd_ppn, m.pt_ppn, physical_page, user_access);
    assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

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

pointer ArchMemory::checkAddressValid(uint32 vaddress_to_check) const
{
  return checkAddressValid(page_dir_page_, vaddress_to_check);
}

pointer ArchMemory::checkAddressValid(ppn_t pd, uint32 vaddress_to_check)
{
  ArchMemoryMapping m = resolveMapping(pd, vaddress_to_check / PAGE_SIZE);
  if (m.page != 0)
  {
    debug(A_MEMORY, "checkAddressValid, pd: %#zx, vaddr: %#zx -> true\n", pd, vaddress_to_check);
    return m.page | (vaddress_to_check % m.page_size);
  }
  else
  {
    debug(A_MEMORY, "checkAddressValid, pd: %#zx, vaddr: %#zx -> false\n", pd, vaddress_to_check);
    return 0;
  }
}

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage) const
{
  return resolveMapping(page_dir_page_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(ppn_t pd, vpn_t vpage)
{
  assert(pd);
  ArchMemoryMapping m;

  VAddr a{vpage*PAGE_SIZE};

  m.pti = a.pti;
  m.pdi = a.pdi;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, vpn: %#zx, pdi: %zx(%zu), pti: %zx(%zu)\n", vpage, m.pdi, m.pdi, m.pti, m.pti);
  }

  assert(pd < PageManager::instance().getTotalNumPages());
  m.pd = (PageDirEntry*) getIdentAddressOfPPN(pd);
  m.pt = 0;
  m.page = 0;
  m.pd_ppn = pd;
  m.pt_ppn = 0;
  m.page_ppn = 0;
  m.page_size = 0;

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

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
      debug(A_MEMORY, "resolveMapping, vpn: %#zx, pd[%s]: %#zx, pt[%s]: %#zx, page[%s]: %#zx, size: %#zx\n",
            vpage,
            (m.pd ? "P" : "-"), m.pd_ppn,
            (m.pt ? "P" : "-"), m.pt_ppn,
            (m.page ? "P" : "-"), m.page_ppn,
            m.page_size);
  }

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
        *physical_pte_page = page_directory[pde_vpn].pt.page_ppn;
      PageTableEntry *pte_base = (PageTableEntry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_ppn);
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

bool ArchMemory::mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  debug(A_MEMORY, "Map kernel page %#zx -> PPN %#zx, alloc new pages: %u, mmio: %u\n", virtual_page, physical_page, can_alloc_pages, memory_mapped_io);
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getKernelPagingStructureRootVirt()) / PAGE_SIZE), virtual_page);

  if (m.page_size)
  {
      return false; // Page already mapped
  }

  m.pd = getKernelPagingStructureRootVirt();

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

  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax"); // TODO: flushing caches after mapping a fresh page is pointless

  return true;
}

void ArchMemory::unmapKernelPage(vpn_t virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getKernelPagingStructureRootVirt()) / PAGE_SIZE), virtual_page);
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
    return page_dir_page_ * PAGE_SIZE;
}

PageDirEntry* ArchMemory::getKernelPagingStructureRootVirt()
{
  return kernel_page_directory;
}

ArchMemory& ArchMemory::kernelArchMemory()
{
    static ArchMemory kernel_arch_mem((size_t)ArchMemory::getKernelPagingStructureRootPhys()/PAGE_SIZE);
    return kernel_arch_mem;
}
