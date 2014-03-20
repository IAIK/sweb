/**
 * @file ArchMemory.cpp
 *
 */

#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "assert.h"
#include "ArchCommon.h"
#include "PageManager.h"

extern "C" uint32 kernel_page_directory_start;

ArchMemory::ArchMemory()
{
  page_map_level_4_ = PageManager::instance()->getFreePhysicalPage();
  PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  ArchCommon::memcpy((pointer) new_pml4,(pointer)kernel_page_map_level_4, PAGE_SIZE);
  bzero(new_pml4,PAGE_SIZE / 2); // should be zero, this is just for safety
}

template<typename T>
bool ArchMemory::checkAndRemove(pointer map_ptr, uint64 index)
{
  T* map = (T*) map_ptr;
  kprintfd("%s: page %x index %x\n", __PRETTY_FUNCTION__, map, index);
  ((uint64*)map)[index] = 0;
  for (uint64 i = 0; i < PAGE_DIR_ENTRIES; i++)
  {
    if (map[index].present != 0)
      return false;
  }
  return true;
}

bool ArchMemory::unmapPage(uint64 virtual_page)
{
  PageMapLevel4Entry* pml4p = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);

  assert(m.page_ppn != 0 && m.page_size == PAGE_SIZE);
  bool empty = checkAndRemove<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti);
  if (empty)
    empty = checkAndRemove<PageDirPageEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi);
  if (empty)
    empty = checkAndRemove<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti);
  if (empty)
    empty = checkAndRemove<PageMapLevel4Entry>(getIdentAddressOfPPN(m.pml4_ppn), m.pml4i);

  assert(false); // you should never get here
}


template<typename T>
bool ArchMemory::insert(pointer map_ptr, uint64 index, uint64 ppn, uint64 size, uint64 user_access, uint64 writeable)
{
  T* map = (T*) map_ptr;
  kprintfd("%s: page %x index %x ppn %x user_access %x size %x\n", __PRETTY_FUNCTION__, map, index, ppn, user_access, size);
  ArchCommon::bzero(getIdentAddressOfPPN(ppn), PAGE_SIZE);
  assert(((uint64*)map)[index] == 0);
  map[index].size = size;
  map[index].writeable = writeable;
  map[index].page_ppn = ppn;
  map[index].user_access = user_access;
  map[index].present = 1;
  return true;
}

bool ArchMemory::mapPage(uint64 virtual_page, uint64 physical_page, uint32 user_access, uint32 page_size)
{
  kprintfd("%x %x %x %x %x\n",page_map_level_4_, virtual_page, physical_page, user_access, page_size);
  PageMapLevel4Entry* pml4p = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  kprintfd("%x\n", *pml4p);
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);

  if (m.pdpt_ppn == 0)
  {
    m.pdpt_ppn = PageManager::instance()->getFreePhysicalPage();
    insert<PageMapLevel4Entry>((pointer) m.pml4, m.pml4i, m.pdpt_ppn, 0, 1, 1);
  }

  if (m.pd_ppn == 0)
  {
    if (page_size == PAGE_SIZE * PAGE_TABLE_ENTRIES * PAGE_DIR_ENTRIES)
    {
      return insert<PageDirPointerTablePageEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdi, physical_page, 1,
                                                     user_access, 1);
    }
    else
    {
      m.pd_ppn = PageManager::instance()->getFreePhysicalPage();
      insert<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti, m.pd_ppn, 0, 1, 1);
    }
  }

  if (m.pt_ppn == 0)
  {
    if (page_size == PAGE_SIZE * PAGE_TABLE_ENTRIES)
    {
      return insert<PageDirPageEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi, physical_page, 1, user_access, 1);
    }
    else if (m.pd == 0)
    {
      m.pt_ppn = PageManager::instance()->getFreePhysicalPage();
      insert<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi, m.pt_ppn, 0, 1, 1);
    }
  }

  if (m.page_ppn == 0 && page_size == PAGE_SIZE)
  {
    return insert<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti, physical_page, 0, user_access, 1);
  }
  assert(false); // you should never get here
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
        if (pdpt[pdpti].pd.present && !pdpt[pdpti].pd.size) // not 1gb page ?
        {
          PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[pdpti].pd.page_ppn);
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (pd[pdi].pt.present && !pd[pdi].pt.size) // not 2mb page ?
            {
              PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
              for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
              {
                if (pt[pti].present)
                {
                  pt[pti].present = 0;
                  PageManager::instance()->freePage(pt[pti].page_ppn);
                }
              }
              pd[pdi].pt.present = 0;
              PageManager::instance()->freePage(pd[pdi].pt.page_ppn);
            }
          }
          pdpt[pdpti].pd.present = 0;
          PageManager::instance()->freePage(pdpt[pdpti].pd.page_ppn);
        }
      }
      pml4[pml4i].present = 0;
      PageManager::instance()->freePage(pml4[pml4i].page_ppn);
    }
  }
//  {
//    if (page_directory[pde_vpn].pt.present)
//    {
//      if (page_directory[pde_vpn].page.use_2_m_pages)
//      {
//        page_directory[pde_vpn].page.present=0;
//          for (uint32 p=0;p<1024;++p)
//            PageManager::instance()->freePage(page_directory[pde_vpn].page.page_ppn*1024 + p);
//      }
//      else
//      {
//        page_table_entry *pte_base = (page_table_entry *) getIdentAddressOfPPN(page_directory[pde_vpn].pt.page_ppn);
//        for (uint32 pte_vpn=0; pte_vpn < PAGE_TABLE_ENTRIES; ++pte_vpn)
//        {
//          if (pte_base[pte_vpn].present)
//          {
//            pte_base[pte_vpn].present = 0;
//            PageManager::instance()->freePage(pte_base[pte_vpn].page_ppn);
//          }
//        }
//        page_directory[pde_vpn].pt.present=0;
//        PageManager::instance()->freePage(page_directory[pde_vpn].pt.page_ppn);
//      }
//    }
//  }
//  PageManager::instance()->freePage(physical_page_directory_page);
}

bool ArchMemory::checkAddressValid(uint64 vaddress_to_check)
{
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, vaddress_to_check / PAGE_SIZE);
  if (m.page != 0)
  {
    kprintfd("checkAddressValid %x and %x -> true", page_map_level_4_, vaddress_to_check);
    return true;
  }
  else
  {
    kprintfd("checkAddressValid %x and %x -> false", page_map_level_4_, vaddress_to_check);
    return false;
  }
  if (m.pml4 && m.pml4[m.pml4i].present)
  {
    if (m.pdpt && m.pdpt[m.pdpti].pd.present && !m.pdpt[m.pdpti].pd.size) // 1gb page ?
    {
      if (m.pd && m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size && m.pt[m.pti].present) // 2mb page ?
          return true;
      else if (m.pd && m.pd[m.pdi].page.present)
        return true;
    }
    else if (m.pdpt && m.pdpt[m.pdpti].page.present)
      return true;
  }
}

uint64 ArchMemory::get_PAddr_Of_VAddr_In_KernelMapping(uint64 virtual_addr)
{
  uint64 physical_addr;
  uint32 page_size = get_PPN_Of_VPN_In_KernelMapping(virtual_addr / PAGE_SIZE, &physical_addr);
  return physical_addr * page_size + (virtual_addr % PAGE_SIZE);
}

ArchMemoryMapping ArchMemory::resolveMapping(uint64 pml4,uint64 vpage)
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

  assert(pml4 <= 2048);
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
      if (m.pd_ppn > 2048)
      {
        kprintfd("%x\n", m.pd_ppn);
      }
      assert(m.pd_ppn <= 2048);
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pdpt[m.pdpti].pd.page_ppn);
      if (m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size) // 2mb page ?
      {
        m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
        assert(m.pt_ppn <= 2048);
        m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pd[m.pdi].pt.page_ppn);
        if (m.pt[m.pti].present)
        {
          m.page = getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
          m.page_ppn = m.pt[m.pti].page_ppn;
          assert(m.page_ppn <= 2048);
          m.page_size = PAGE_SIZE;
        }
      }
      else if (m.pd[m.pdi].page.present)
      {
        m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES;
        m.page_ppn = m.pd[m.pdi].page.page_ppn;
        assert(m.page_ppn <= 2048);
        m.page = getIdentAddressOfPPN(m.pd[m.pdi].page.page_ppn);
      }
    }
    else if (m.pdpt[m.pdpti].page.present)
    {
      m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES * PAGE_DIR_ENTRIES;
      m.page_ppn = m.pdpt[m.pdpti].page.page_ppn;
      assert(m.page_ppn <= 2048);
      m.page = getIdentAddressOfPPN(m.pdpt[m.pdpti].page.page_ppn);
    }
  }
  return m;
}

size_t ArchMemory::get_PPN_Of_VPN_In_KernelMapping(size_t virtual_page, size_t *physical_page, size_t *physical_pte_page)
{
  ArchMemoryMapping m = resolveMapping(PML4_KERNEL_PAGE, virtual_page);
  if (physical_page)
    *physical_page = m.page_ppn;
  if (physical_pte_page)
    *physical_pte_page = m.pt_ppn;
  return m.page_size;
}
