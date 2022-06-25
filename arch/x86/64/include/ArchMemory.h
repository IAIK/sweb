#pragma once

#include "types.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "EASTL/atomic.h"

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
    ArchMemory(ppn_t pml4_ppn);
    ~ArchMemory();


  [[nodiscard]]
  bool mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access);
  bool unmapPage(vpn_t virtual_page);

  [[nodiscard]]
  static bool mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages = false, bool memory_mapped_io = false);
  static void unmapKernelPage(vpn_t virtual_page, bool free_page = true);

/**
 * Takes a Physical Page Number in Real Memory and returns a virtual address than
 * can be used to access given page
 * @param ppn Physical Page Number
 * @param page_size Optional, defaults to 4k pages, but you ned to set it to
 * 1024*4096 if you have a 4m page number
 * @return Virtual Address above 3GB pointing to the start of a memory segment that
 * is mapped to the physical page given
 */
  static pointer getIdentAddressOfPPN(ppn_t ppn, size_t page_size = PAGE_SIZE);
  static pointer getIdentAddress(size_t address); // TODO: rename to distinguish it from getIdentAddressOfPPN

/**
 * Checks if a given Virtual Address is valid and is mapped to physical memory
 * @param pml4 pml4 ppn
 * @param vaddress_to_check Virtual Address we want to check
 * @return physical address if the virtual address is mapped, zero otherwise
 */
  pointer checkAddressValid(size_t vaddress_to_check) const;
  static pointer checkAddressValid(ppn_t pml4, size_t vaddress_to_check);

  const ArchMemoryMapping resolveMapping(vpn_t vpage) const;
  static const ArchMemoryMapping resolveMapping(ppn_t pml4, vpn_t vpage);

  size_t getPagingStructureRootPhys() const;
  size_t getValueForCR3() const;



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
  static size_t get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, ppn_t* physical_page, ppn_t* physical_pte_page=0);

  static PageMapLevel4Entry* getKernelPagingStructureRootVirt();
  static size_t getKernelPagingStructureRootPhys();
  static void loadPagingStructureRoot(size_t cr3_value);

  static void flushLocalTranslationCaches(size_t addr);
  static void flushAllTranslationCaches(size_t addr);

  static void initKernelArchMem();

private:
  ArchMemory &operator=(ArchMemory const &src) = delete;

  template<typename T, size_t NUM_ENTRIES>
  static bool tableEmpty(T* table);

  template<typename T>
  static void removeEntry(T* map, size_t index);

  template <typename T>
  static void insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable, bool memory_mapped_io);


  ppn_t page_map_level_4_;
};

extern ArchMemory kernel_arch_mem;
