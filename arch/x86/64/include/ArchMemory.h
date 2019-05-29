#pragma once

#include "types.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "uatomic.h"

extern PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(0x1000)));

class ArchMemoryMapping
{
  public:
    PageMapLevel4Entry* pml4;
    PageDirPointerTableEntry* pdpt;
    PageDirEntry* pd;
    PageTableEntry* pt;
    pointer page;

    ppn_t pml4_ppn;
    ppn_t pdpt_ppn;
    ppn_t pd_ppn;
    ppn_t pt_ppn;
    ppn_t page_ppn;

    size_t page_size;

    uint64 pml4i;
    uint64 pdpti;
    uint64 pdi;
    uint64 pti;
};

class ArchMemory
{
public:
    ArchMemory();

/** 
 *
 * maps a virtual page to a physical page (pde and pte need to be set up first)
 *
 * @param physical_page_directory_page Real Page where the PDE to work on resides
 * @param virtual_page 
 * @param physical_page
 * @param user_access PTE User/Supervisor Flag, governing the binary Paging
 * Privilege Mechanism
 */
  __attribute__((warn_unused_result)) bool mapPage(vpn_t virtual_page, ppn_t physical_page, size_t user_access);

/**
 * removes the mapping to a virtual_page by marking its PTE Entry as non valid
 *
 * @param physical_page_directory_page Real Page where the PDE to work on resides
 * @param virtual_page which will be invalidated
 */
  bool unmapPage(vpn_t virtual_page);

  ~ArchMemory();

/**
 * Takes a Physical Page Number in Real Memory and returns a virtual address than
 * can be used to access given page
 * @param ppn Physical Page Number
 * @param page_size Optional, defaults to 4k pages, but you ned to set it to
 * 1024*4096 if you have a 4m page number
 * @return Virtual Address above 3GB pointing to the start of a memory segment that
 * is mapped to the physical page given
 */
    static pointer getIdentAddressOfPPN(ppn_t ppn, size_t page_size=PAGE_SIZE)
    {
      return 0xFFFFF00000000000ULL | (ppn * page_size);
    }

    static pointer getIdentAddress(size_t address)
    {
      return 0xFFFFF00000000000ULL | (address);
    }

/**
 * Checks if a given Virtual Address is valid and is mapped to real memory
 * @param vaddress_to_check Virtual Address we want to check
 * @return true: if mapping exists\nfalse: if the given virtual address is unmapped
 * and accessing it would result in a pageFault
 */
  pointer checkAddressValid(size_t vaddress_to_check);

/**
 * Takes a virtual_page and search through the pageTable and pageDirectory for the
 * physical_page it refers to
 * to get a physical address (which you can only use by adding 3gb to it) multiply
 * the &physical_page with the return value
 * @param virtual_page virtual Page to look up
 * @param *physical_page Pointer to the result
 * @param *physical_pte_page optional: Pointer to physical page number of used PTE, or unchanged if 4MiB Page
 * @return 0: if the virtual page doesn't map to any physical page\notherwise
 * returns the page size in byte (4096 for 4KiB pages or 4096*1024 for 4MiB pages)
 */
  static size_t get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, ppn_t *physical_page, ppn_t *physical_pte_page=0);
  const ArchMemoryMapping resolveMapping(vpn_t vpage);
  static const ArchMemoryMapping resolveMapping(ppn_t pml4, vpn_t vpage);

/**
 *
 * maps a virtual page to a physical page in kernel mapping
 *
 * @param virtual_page
 * @param physical_page
 */
  static void mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages = false, bool memory_mapped_io = false);

/**
 * removes the mapping to a virtual_page by marking its PTE Entry as non valid
 * in kernel mapping
 *
 * @param virtual_page which will be invalidated
 */
  static void unmapKernelPage(vpn_t virtual_page, bool free_page = true);


  uint64 getRootOfPagingStructure();
  uint64 getValueForCR3();
  static PageMapLevel4Entry* getRootOfKernelPagingStructure();
  static void loadPagingStructureRoot(size_t cr3_value);

  static void flushLocalTranslationCaches(size_t addr);
  static void flushAllTranslationCaches(size_t addr);

  static const size_t RESERVED_START = 0xFFFFFFFF80000ULL;
  static const size_t RESERVED_END = 0xFFFFFFFFC0000ULL;

  ppn_t page_map_level_4_;

private:

/** 
 * Adds a page directory entry to the given page directory.
 * (In other words, adds the reference to a new page table to a given
 * page directory.)
 *
 * @param physical_page_directory_page physical page containing the target PD.
 * @param pde_vpn Index of the PDE (i.e. the page table) in the PD.
 * @param physical_page_table_page physical page of the new page table.
 */
  template <typename T>
  static void insert(pointer map_ptr, uint64 index, uint64 ppn, uint64 bzero, uint64 size, uint64 user_access, uint64 writeable);

/**
 * Removes a page directory entry from a given page directory if it is present
 * in the first place. Futhermore, the target page table is assured to be
 * empty.
 *
 * @param physical_page_directory_page physical page containing the target PD.
 * @param pde_vpn Index of the PDE (i.e. the page table) in the PD.
 */
  template<typename T>
  static bool checkAndRemove(pointer map_ptr, uint64 index);

  ArchMemory(ArchMemory const &src);
  ArchMemory &operator=(ArchMemory const &src);

};
