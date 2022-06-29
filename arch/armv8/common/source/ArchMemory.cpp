#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "kprintf.h"
#include "assert.h"
#include "PageManager.h"
#include "offsets.h"
#include "kstring.h"

Level1Entry kernel_paging_level1[KERNEL_LEVEL1_TABLES * LEVEL1_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
//one page for ident and one for the kernel pages
Level2Entry kernel_paging_level2[KERNEL_LEVEL2_TABLES * LEVEL2_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
//reserve 16MB of possible kernel memory
Level3Entry kernel_paging_level3[KERNEL_LEVEL3_TABLES * LEVEL3_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

uint16 ASID_COUNTER = 0;

ArchMemory kernel_arch_mem((size_t)ArchMemory::getKernelPagingStructureRootPhys()/PAGE_SIZE);

ArchMemory::ArchMemory()
{
  paging_root_page_ = PageManager::instance()->allocPPN(PAGE_SIZE);
  address_space_id = ASID_COUNTER++;

  debug(A_MEMORY, "ArchMemory::ArchMemory(): Got new Page no. %zx\n", paging_root_page_);
}

ArchMemory::ArchMemory(ppn_t paging_root_page) :
    paging_root_page_(paging_root_page)
{
    debug(A_MEMORY, "ArchMemory::ArchMemory(%zx)\n", paging_root_page_);
    address_space_id = ASID_COUNTER++;
}

void ArchMemory::loadPagingStructureRoot(size_t ttbr0_value)
{
    __asm__ __volatile__("MSR TTBR0_EL1 , %[ttbr0]\n"
                         ::[ttbr0]"r"(ttbr0_value));
}

template<typename T, size_t NUM_ENTRIES>
bool ArchMemory::tableEmpty(T* table)
{
    for (size_t i = 0; i < NUM_ENTRIES; i++)
    {
        if (table[i].entry_descriptor_type)
            return false;
    }
    return true;
}

template<typename T>
void ArchMemory::removeEntry(T* table, size_t index)
{
    assert(table[index].entry_descriptor_type);

    table[index].entry_descriptor_type = 0;
    memset(&table[index], 0, sizeof(table[index]));
}

bool ArchMemory::unmapPage(size_t virtual_page)
{
    ArchMemoryMapping m = resolveMapping(virtual_page);
    debug(A_MEMORY, "Unmap vpn %zx => %zx[%zx]->%zx[%zx]->%zx[%zx]->%zx\n",
          virtual_page,
          m.level1_ppn, m.level1_index,
          m.level2_ppn, m.level2_index,
          m.level3_ppn, m.level3_index,
          m.page_ppn);

    assert(m.page_ppn != 0 && m.page_size == PAGE_SIZE && m.level3_entry[m.level3_index].entry_descriptor_type == ENTRY_DESCRIPTOR_PAGE);

    removeEntry(m.level3_entry, m.level3_index);

    bool l2_empty = false;
    bool l3_empty = tableEmpty<Level3Entry, LEVEL3_ENTRIES>(m.level3_entry);

    if(l3_empty)
    {
        l2_empty = tableEmpty<Level12TableEntry, LEVEL2_ENTRIES>(&m.level2_entry->table);
    }

    flushAllTranslationCaches(virtual_page * PAGE_SIZE); // Needs to happen after page table entries have been modified but before PPNs are freed

    PageManager::instance()->freePPN(m.page_ppn);
    if(l3_empty)   { PageManager::instance()->freePPN(m.level3_ppn);   }
    if(l2_empty)   { PageManager::instance()->freePPN(m.level2_ppn);   }

    return true;
}

bool ArchMemory::mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access)
{
    ArchMemoryMapping m = resolveMapping(paging_root_page_, virtual_page);
    debug(A_MEMORY, "Map vpn %zx => %zx[%zx]->%zx[%zx]->%zx[%zx]->%zx to ppn: %llx, user: %u\n",
          virtual_page,
          m.level1_ppn, m.level1_index,
          m.level2_ppn, m.level2_index,
          m.level3_ppn, m.level3_index,
          m.page_ppn,
          physical_page,
          user_access);

    assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

    if(m.level2_ppn == 0)
    {
        m.level2_ppn = PageManager::instance()->allocPPN(PAGE_SIZE);
        m.level2_entry = (Level2Entry*)getIdentAddressOfPPN(m.level2_ppn);
        m.level1_entry[m.level1_index].table_address = m.level2_ppn;
        m.level1_entry[m.level1_index].entry_descriptor_type = ENTRY_DESCRIPTOR_PAGE;
    }

    if(m.level3_ppn == 0)
    {
        m.level3_ppn = PageManager::instance()->allocPPN(PAGE_SIZE);
        m.level3_entry = (Level3Entry*)getIdentAddressOfPPN(m.level3_ppn);
        m.level2_entry[m.level2_index].table.table_address = m.level3_ppn;
        m.level2_entry[m.level2_index].table.entry_descriptor_type = ENTRY_DESCRIPTOR_PAGE;
    }

    if (m.page_ppn == 0)
    {
        m.level3_entry[m.level3_index].not_global = 1;
        m.level3_entry[m.level3_index].shareability_field = SHARE_ISH;
        m.level3_entry[m.level3_index].access_permissions = user_access;
        m.level3_entry[m.level3_index].memory_attributes_index = MEMORY_ATTR_STD;
        m.level3_entry[m.level3_index].page_address = physical_page;
        m.level3_entry[m.level3_index].access_flag = ACCESS_FLAG;
        m.level3_entry[m.level3_index].entry_descriptor_type = ENTRY_DESCRIPTOR_PAGE;
        return true;
    }

    return false;
}

ArchMemory::~ArchMemory()
{
    debug(A_MEMORY, "ArchMemory::~ArchMemory(): Freeing ArchMem %zx\n", paging_root_page_);

    Level1Entry * level1_entry = (Level1Entry *)getIdentAddressOfPPN(paging_root_page_);

    for(int level1_index = 0; level1_index < LEVEL1_ENTRIES; level1_index++)
    {
        assert(level1_entry[level1_index].entry_descriptor_type != ENTRY_DESCRIPTOR_BLOCK);
        if(level1_entry[level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
        {
            auto l2_ppn = level1_entry[level1_index].table_address;
            Level2Entry * level2_entry = (Level2Entry *)getIdentAddressOfPPN(level1_entry[level1_index].table_address);
            for(int level2_index = 0; level2_index < LEVEL2_ENTRIES; level2_index++)
            {
                assert(level2_entry[level2_index].table.entry_descriptor_type != ENTRY_DESCRIPTOR_BLOCK);
                if(level2_entry[level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
                {
                    auto l3_ppn = level2_entry[level2_index].table.table_address;
                    Level3Entry * level3_entry = (Level3Entry *)getIdentAddressOfPPN(level2_entry[level2_index].table.table_address);
                    for(int level3_index = 0; level3_index < LEVEL3_ENTRIES; level3_index++)
                    {
                        if(level3_entry[level3_index].entry_descriptor_type == ENTRY_DESCRIPTOR_PAGE)
                        {
                            auto page_ppn = level3_entry[level3_index].page_address;
                            removeEntry(level3_entry, level3_index);
                            PageManager::instance()->freePPN(page_ppn);
                        }
                    }
                    removeEntry(&level2_entry->table, level2_index);
                    PageManager::instance()->freePPN(l3_ppn);
                }
            }
            removeEntry(level1_entry, level1_index);
            PageManager::instance()->freePPN(l2_ppn);
        }
    }

    PageManager::instance()->freePPN(paging_root_page_);
}

template<typename T>
bool ArchMemory::checkAndRemove(pointer table_ptr, size_t index)
{
  T* map = (T*) table_ptr;
  debug(A_MEMORY, "%s: page %p index %zx\n", __PRETTY_FUNCTION__, map, index);

  ((size_t*) map)[index] = 0;

  for (size_t i = 0; i < PAGE_ENTRIES; i++)
  {
    if (map[i].entry_descriptor_type != 0)
      return false;
  }

  return true;
}

pointer ArchMemory::checkAddressValid(size_t vaddress_to_check)
{
    ArchMemoryMapping m = resolveMapping(paging_root_page_, vaddress_to_check / PAGE_SIZE);

    if (m.page != 0)
    {
      debug(A_MEMORY, "checkAddressValid %zx and %zx -> true\n", paging_root_page_, vaddress_to_check);
      return m.page | (vaddress_to_check % m.page_size);
    }
    else
    {
      debug(A_MEMORY, "checkAddressValid %zx and %zx -> false\n", paging_root_page_, vaddress_to_check);
      return 0;
    }
}

size_t ArchMemory::get_PPN_Of_VPN_In_KernelMapping(size_t virtual_page, size_t *physical_page,
                                                   size_t *physical_pte_page)
{
    ArchMemoryMapping m = resolveMapping(getKernelPagingStructureRootPhys() / PAGE_SIZE, virtual_page);

     if (physical_page)
       *physical_page = m.page_ppn;

     if (physical_pte_page)
       *physical_pte_page = m.level3_ppn;

     return m.page_size;
}

const ArchMemoryMapping ArchMemory::resolveMapping(size_t vpage)
{
  return resolveMapping(paging_root_page_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(size_t level1_ppn, size_t vpage)
{
    ArchMemoryMapping m;

    size_t total_num_pages = PageManager::instance()->getTotalNumPages();

    m.level3_index = vpage;
    m.level2_index = m.level3_index / LEVEL3_ENTRIES;
    m.level1_index = m.level2_index / LEVEL2_ENTRIES;

    m.level3_index %= LEVEL3_ENTRIES;
    m.level2_index %= LEVEL2_ENTRIES;
    m.level1_index %= LEVEL1_ENTRIES;

    assert(level1_ppn < total_num_pages);

    m.level1_entry = (Level1Entry*) getIdentAddressOfPPN(level1_ppn);
    m.level2_entry = 0;
    m.level3_entry = 0;

    m.level1_ppn = level1_ppn;
    m.level2_ppn = 0;
    m.level3_ppn = 0;

    m.page = 0;
    m.page_ppn = 0;
    m.page_size = 0;

    if(m.level1_entry[m.level1_index].entry_descriptor_type == 0)
        return m;

    m.level2_ppn = m.level1_entry[m.level1_index].table_address;
    assert(m.level2_ppn < total_num_pages);
    m.level2_entry = (Level2Entry *)getIdentAddressOfPPN(m.level2_ppn);

    if(m.level2_entry[m.level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
    {
        m.level3_ppn = m.level2_entry[m.level2_index].table.table_address;
        assert(m.level3_ppn < total_num_pages);
        m.level3_entry = (Level3Entry *)getIdentAddressOfPPN(m.level3_ppn);

        if(m.level3_entry[m.level3_index].entry_descriptor_type == ENTRY_DESCRIPTOR_PAGE)
        {
            m.page_size = PAGE_SIZE;
            m.page_ppn = m.level3_entry[m.level3_index].page_address;
            assert(m.page_ppn < total_num_pages);
            m.page = getIdentAddressOfPPN(m.page_ppn);
        }
    }
    else if (m.level2_entry[m.level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_BLOCK) //2MB page
    {
        m.page_size = PAGE_ENTRIES * PAGE_SIZE;
        m.page_ppn = m.level2_entry[m.level2_index].block.block_address;
        m.page = getIdentAddressOfPPN(m.page_ppn);
    }
    else if(m.level2_entry[m.level2_index].table.entry_descriptor_type != 0)
    {
        assert(false && "Invalid level2 page table entry type");
    }

    return m;
}

bool ArchMemory::mapKernelPage(size_t virtual_page, size_t physical_page, [[maybe_unused]]bool can_alloc_pages, [[maybe_unused]]bool memory_mapped_io)
{
    ArchMemoryMapping m = resolveMapping(VIRTUAL_TO_PHYSICAL_BOOT((size_t)kernel_paging_level1) / PAGE_SIZE,
                                             virtual_page);
    debug(A_MEMORY, "Map (kernel) %zx => l1: %zx, l2: %zx, l3: %zx, page: %zx, new page: %zx\n",
          virtual_page, m.level1_ppn, m.level2_ppn, m.level3_ppn, m.page_ppn, physical_page);

    if (m.page)
    {
        debug(A_MEMORY, "Map (kernel) %zx, abort: page is already mapped\n", virtual_page);
        return false;
    }

    Level1Entry* level1_entry = kernel_paging_level1;
    assert(level1_entry[m.level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE);
    Level2Entry* level2_entry = (Level2Entry*) getIdentAddressOfPPN(level1_entry[m.level1_index].table_address);
    assert(level2_entry[m.level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE);
    Level3Entry* level3_entry = (Level3Entry*) getIdentAddressOfPPN(level2_entry[m.level2_index].table.table_address);
    assert(level3_entry[m.level3_index].entry_descriptor_type == 0);

    *((size_t*)&(level3_entry[m.level3_index])) = 0;

    level3_entry[m.level3_index].entry_descriptor_type = ENTRY_DESCRIPTOR_PAGE;
    level3_entry[m.level3_index].memory_attributes_index = MEMORY_ATTR_STD;
    level3_entry[m.level3_index].shareability_field = SHARE_ISH;
    level3_entry[m.level3_index].access_flag = ACCESS_FLAG;
    level3_entry[m.level3_index].page_address = physical_page;

    return true;
}

void ArchMemory::unmapKernelPage(size_t virtual_page)
{
    ArchMemoryMapping m =  resolveMapping(VIRTUAL_TO_PHYSICAL_BOOT((size_t)kernel_paging_level1) / PAGE_SIZE,
            virtual_page);

    Level1Entry* level1_entry = kernel_paging_level1;
    assert(level1_entry[m.level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE);
    Level2Entry* level2_entry = (Level2Entry*) getIdentAddressOfPPN(level1_entry[m.level1_index].table_address);
    assert(level2_entry[m.level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE);
    Level3Entry* level3_entry = (Level3Entry*) getIdentAddressOfPPN(level2_entry[m.level2_index].table.table_address);
    assert(level3_entry[m.level3_index].entry_descriptor_type == ENTRY_DESCRIPTOR_PAGE);

    *((size_t*)&(level3_entry[m.level3_index])) = 0;

    PageManager::instance()->freePPN(m.page_ppn);
}

size_t ArchMemory::getRootOfPagingStructure()
{
  return paging_root_page_;
}

Level1Entry* ArchMemory::getKernelPagingStructureRootVirt()
{
    return kernel_paging_level1;
}

size_t ArchMemory::getKernelPagingStructureRootPhys()
{
    return (size_t)VIRTUAL_TO_PHYSICAL_BOOT((size_t)getKernelPagingStructureRootVirt());
}

void ArchMemory::initKernelArchMem()
{
    new (&kernel_arch_mem) ArchMemory((size_t)getKernelPagingStructureRootPhys()/PAGE_SIZE);
    kernel_arch_mem.printMappings();
}


void ArchMemory::printMappings()
{
    size_t tcr_el1 = 0;
    asm volatile ("mrs %0, tcr_el1" : "=r" (tcr_el1));

    debug(A_MEMORY, "tcr_el1: %zx\n", tcr_el1);

    debug(A_MEMORY, "Page mappings for %zx\n", paging_root_page_);

    Level1Entry * level1_entry = (Level1Entry *)getIdentAddressOfPPN(paging_root_page_);

    debug(A_MEMORY, "Root page - %zx\n", paging_root_page_);

    // L0 skipped
    for(int level1_index = 0; level1_index < LEVEL1_ENTRIES; level1_index++)
    {
        VAddr start{TTBR1_LIMIT_LOW};
        VAddr end{TTBR1_LIMIT_LOW};
        start.l1i = level1_index;
        end.l1i = level1_index+1;

        if (level1_entry[level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
        {
            debug(A_MEMORY, "L1 -- [%12llx, %12llx)] %5zx[%5x]->%5zx\n",
                  start.addr, end.addr, paging_root_page_,
                  level1_index, level1_entry[level1_index].table_address);
        }
        else if (level1_entry[level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_BLOCK)
        {
            debug(A_MEMORY, "L1 -- [%12llx, %12llx)] %5zx[%5x]->%5zx BLOCK\n",
                  start.addr, end.addr, paging_root_page_,
                  level1_index, level1_entry[level1_index].table_address);
            continue;
        }

        if(level1_entry[level1_index].entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
        {
            Level2Entry * level2_entry = (Level2Entry *)getIdentAddressOfPPN(level1_entry[level1_index].table_address);
            for(int level2_index = 0; level2_index < LEVEL2_ENTRIES; level2_index++)
            {
                VAddr start{TTBR1_LIMIT_LOW};
                VAddr end{TTBR1_LIMIT_LOW};
                start.l1i = level1_index;
                end.l1i = level2_index ==  LEVEL1_ENTRIES-1 ? level1_index+1 : level1_index;
                start.l2i = level2_index;
                end.l2i = level2_index+1;

                if (level2_entry[level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
                {
                    debug(A_MEMORY, "L2 -- [%12llx, %12llx)] %5zx[%5x]->%5zx[%5x]->%5zx\n",
                          start.addr, end.addr, paging_root_page_, level1_index, level1_entry[level1_index].table_address, level2_index, level2_entry[level2_index].table.table_address);
                }
                else if (level2_entry[level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_BLOCK)
                {
                    debug(A_MEMORY, "L2 -- [%12llx, %12llx)] %5zx[%5x]->%5zx[%5x]->%5x BLOCK\n",
                          start.addr, end.addr, paging_root_page_, level1_index, level1_entry[level1_index].table_address, level2_index, level2_entry[level2_index].block.block_address);
                    continue;
                }


                if(level2_entry[level2_index].table.entry_descriptor_type == ENTRY_DESCRIPTOR_TABLE)
                {
                    Level3Entry * level3_entry = (Level3Entry *)getIdentAddressOfPPN(level2_entry[level2_index].table.table_address);
                    for(int level3_index = 0; level3_index < LEVEL3_ENTRIES; level3_index++)
                    {
                        VAddr start{TTBR1_LIMIT_LOW};
                        VAddr end{TTBR1_LIMIT_LOW};
                        start.l1i = level1_index;
                        end.l1i = level2_index ==  LEVEL2_ENTRIES-1 ? level1_index+1 : level1_index;
                        start.l2i = level2_index;
                        end.l2i = level3_index ==  LEVEL3_ENTRIES-1 ? level2_index+1 : level2_index;
                        start.l3i = level3_index;
                        end.l3i = level3_index+1;


                        if(level3_entry[level3_index].entry_descriptor_type == ENTRY_DESCRIPTOR_PAGE)
                        {
                            debug(A_MEMORY, "L3 -- [%12llx, %12llx)] %5zx[%5x]->%5zx[%5x]->%5zx[%5x]->%5zx PAGE\n",
                                  start.addr, end.addr, paging_root_page_,
                                  level1_index,
                                  level1_entry[level1_index].table_address,
                                  level2_index,
                                  level2_entry[level2_index].table.table_address,
                                  level3_index,
                                  level3_entry[level3_index].page_address);
                        }
                    }
                }
            }
        }
    }
}


void ArchMemory::flushLocalTranslationCaches(size_t addr)
{
    if(A_MEMORY & OUTPUT_ADVANCED)
    {
        debug(A_MEMORY, "CPU %zx flushing translation caches for address %zx\n", ArchMulticore::getCpuID(), addr);
    }

    asm volatile("DSB ISHST\n" // Data synchronization barrier
                 "TLBI VAAE1, %[vpn]\n" // Invalidate all TLB entries for vpn
                 ::[vpn]"r"(addr >> 12));
}

void ArchMemory::flushAllTranslationCaches(size_t addr)
{
    flushLocalTranslationCaches(addr);

    // TODO: not yet implemented
}
