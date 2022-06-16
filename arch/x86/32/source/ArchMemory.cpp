#include "ArchMemory.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "kstring.h"
#include "offsets.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "ArchCommon.h"

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
                PageTableEntry *pt = (PageTableEntry *) getIdentAddressOfPPN(pd[pdi].pt.page_table_ppn);
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
  return checkAddressValid(page_dir_page_, vaddress_to_check);
}

pointer ArchMemory::checkAddressValid(size_t pd, uint32 vaddress_to_check)
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

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage)
{
  return resolveMapping(page_dir_page_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(ppn_t pd, vpn_t vpage)
{
  ArchMemoryMapping m;

  VAddr a{vpage*PAGE_SIZE};

  m.pti = a.pti;
  m.pdi = a.pdi;

  if(A_MEMORY & OUTPUT_ADVANCED)
  {
    debug(A_MEMORY, "resolveMapping, vpn: %#zx, pdi: %zx(%zu), pti: %zx(%zu)\n", vpage, m.pdi, m.pdi, m.pti, m.pti);
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

void ArchMemory::mapKernelPage(uint32 virtual_page, uint32 physical_page, bool can_alloc_pages, bool memory_mapped_io)
{
  debug(A_MEMORY, "Map kernel page %#zx -> PPN %#zx, alloc new pages: %u, mmio: %u\n", virtual_page, physical_page, can_alloc_pages, memory_mapped_io);
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getRootOfKernelPagingStructure()) / PAGE_SIZE), virtual_page);

  assert(!m.page_size && "Page already mapped");

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

  asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax"); // TODO: flushing caches after mapping a fresh page is pointless
}

void ArchMemory::unmapKernelPage(uint32 virtual_page, bool free_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(getRootOfKernelPagingStructure()) / PAGE_SIZE), virtual_page);
  assert(m.page && (m.page_size == PAGE_SIZE));

  if(free_page)
  {
    PageManager::instance()->freePPN(m.page_ppn);
  }
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

eastl::atomic<size_t> shootdown_request_counter;

void ArchMemory::flushAllTranslationCaches(size_t addr)
{

        assert(ArchMulticore::cpu_list_.size() >= 1);
        eastl::vector<TLBShootdownRequest> shootdown_requests{ArchMulticore::cpu_list_.size()};
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

                        assert(cpu->lapic->ID() == cpu_id);
                        asm("mfence\n");
                        cpu_info.lapic->sendIPI(99, *cpu->lapic, true);
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
