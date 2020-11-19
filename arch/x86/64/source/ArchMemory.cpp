#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "kstring.h"
#include "ArchMulticore.h"
#include "ArchCommon.h"

PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(0x1000)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(0x1000)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(0x1000)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(0x1000)));


ArchMemory::ArchMemory()
{
  page_map_level_4_ = PageManager::instance()->allocPPN();
  PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
  memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety
}

template<typename T>
bool ArchMemory::checkAndRemove(pointer map_ptr, uint64 index)
{
  T* map = (T*) map_ptr;
  debug(A_MEMORY, "%s: page %p index %llx\n", __PRETTY_FUNCTION__, map, index);
  ((uint64*) map)[index] = 0;
  for (uint64 i = 0; i < PAGE_DIR_ENTRIES; i++)
  {
    if (map[i].present != 0)
      return false;
  }
  return true;
}

bool ArchMemory::unmapPage(vpn_t virtual_page)
{
  ArchMemoryMapping m = resolveMapping(virtual_page);

  assert(m.page_ppn != 0 && m.page_size == PAGE_SIZE && m.pt[m.pti].present);
  m.pt[m.pti].present = 0;

  ((uint64*)m.pt)[m.pti] = 0;

  bool pt_empty = checkAndRemove<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti);
  bool pd_empty = false;
  bool pdpt_empty = false;

  if (pt_empty)
  {
    pd_empty = checkAndRemove<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi);
  }
  if (pd_empty)
  {
    pdpt_empty = checkAndRemove<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti);
  }
  if (pdpt_empty)
  {
    checkAndRemove<PageMapLevel4Entry>(getIdentAddressOfPPN(m.pml4_ppn), m.pml4i);
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

pointer ArchMemory::checkAddressValid(size_t pml4, size_t vaddress_to_check)
{
  ArchMemoryMapping m = resolveMapping(pml4, vaddress_to_check / PAGE_SIZE);
  if (m.page != 0)
  {
    debug(A_MEMORY, "checkAddressValid, pml4: %#zx, vaddr: %#zx -> true\n", pml4, vaddress_to_check);
    return m.page | (vaddress_to_check % m.page_size);
  }
  else
  {
    debug(A_MEMORY, "checkAddressValid, pml4: %#zx, vaddr: %#zx -> false\n", pml4, vaddress_to_check);
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

void ArchMemory::mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  //debug(A_MEMORY, "mapKernelPage, vpn: %zx, ppn: %zx\n", virtual_page, physical_page);
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getRootOfKernelPagingStructure()) / PAGE_SIZE), virtual_page);

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
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

void ArchMemory::unmapKernelPage(size_t virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                             virtual_page);

  assert(m.page && (m.page_size == PAGE_SIZE));

  memset(&m.pt[m.pti], 0, sizeof(m.pt[m.pti]));
  if(free_page)
  {
    PageManager::instance()->freePPN(m.pt[m.pti].page_ppn);
  }
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

uint64 ArchMemory::getRootOfPagingStructure()
{
  return page_map_level_4_;
}

PageMapLevel4Entry* ArchMemory::getRootOfKernelPagingStructure()
{
  return kernel_page_map_level_4;
}

void ArchMemory::loadPagingStructureRoot(size_t cr3_value)
{
  __asm__ __volatile__("movq %[cr3_value], %%cr3\n"
                       ::[cr3_value]"r"(cr3_value));
}

uint64 ArchMemory::getValueForCR3()
{
  return getRootOfPagingStructure() * PAGE_SIZE;
}


void ArchMemory::flushLocalTranslationCaches(size_t addr)
{
  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "CPU %zx flushing translation caches for address %zx\n", ArchMulticore::getCpuID(), addr);
  }
  __asm__ __volatile__("invlpg %[addr]\n"
                       :
                       :[addr]"m"(*(char*)addr));
}

ustl::atomic<size_t> shootdown_request_counter;

void ArchMemory::flushAllTranslationCaches(size_t addr)
{

        assert(ArchMulticore::cpu_list_.size() >= 1);
        ustl::vector<TLBShootdownRequest> shootdown_requests{ArchMulticore::cpu_list_.size()};
        //TLBShootdownRequest shootdown_requests[ArchMulticore::cpu_list_.size()]; // Assuming the kernel stack is large enough as long as we only have a few CPUs

        bool interrupts_enabled = ArchInterrupts::disableInterrupts();
        flushLocalTranslationCaches(addr);

        auto orig_cpu = ArchMulticore::getCpuID();

        ((char*)ArchCommon::getFBPtr())[2*80*2 + orig_cpu*2] = 's';

        /////////////////////////////////////////////////////////////////////////////////
        // Thread may be re-scheduled on a different CPU while sending TLB shootdowns  //
        // This is fine, since a context switch also invalidates the TLB               //
        // A CPU sending a TLB shootdown to itself is also fine                        //
        /////////////////////////////////////////////////////////////////////////////////
        size_t request_id = ++shootdown_request_counter;

        for(auto& r : shootdown_requests)
        {
                r.addr = addr;
                r.ack = 0;
                r.target = (size_t)-1;
                r.next = nullptr;
                r.orig_cpu = orig_cpu;
                r.request_id = request_id;
        }


        shootdown_requests[orig_cpu].ack |= (1 << orig_cpu);

        size_t sent_shootdowns = 0;

        for(auto& cpu : ArchMulticore::cpu_list_)
        {
                size_t cpu_id = cpu->getCpuID();
                shootdown_requests[cpu_id].target = cpu_id;
                if(cpu->getCpuID() != orig_cpu)
                {
                        debug(A_MEMORY, "CPU %zx Sending TLB shootdown request %zx for addr %zx to CPU %zx\n", ArchMulticore::getCpuID(), shootdown_requests[cpu_id].request_id, addr, cpu_id);
                        assert(ArchMulticore::getCpuID() == orig_cpu);
                        assert(cpu_id != orig_cpu);
                        sent_shootdowns |= (1 << cpu_id);

                        TLBShootdownRequest* expected_next = nullptr;
                        do
                        {
                                shootdown_requests[cpu_id].next = expected_next;
                        } while(!cpu->tlb_shootdown_list.compare_exchange_weak(expected_next, &shootdown_requests[cpu_id]));

                        assert(cpu->lapic.ID() == cpu_id);
                        asm("mfence\n");
                        cpu_info.lapic.sendIPI(99, cpu->lapic, true);
                }
        }
        assert(!(sent_shootdowns & (1 << orig_cpu)));

        debug(A_MEMORY, "CPU %zx sent %zx TLB shootdown requests, waiting for ACKs\n", ArchMulticore::getCpuID(), sent_shootdowns);

        if(interrupts_enabled) ArchInterrupts::enableInterrupts();

        for(auto& r : shootdown_requests)
        {
                if(r.target != orig_cpu)
                {
                        do
                        {
                                assert((r.ack.load() &~ sent_shootdowns) == 0);
                        }
                        while(r.ack.load() == 0);
                }
        }

        ((char*)ArchCommon::getFBPtr())[2*80*2 + orig_cpu*2] = ' ';
}
