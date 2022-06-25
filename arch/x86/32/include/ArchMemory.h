#pragma once

#include "types.h"
#include "paging-definitions.h"

#define RESOLVEMAPPING(pd,vpage) ;\
  PageDirEntry* page_directory = (PageDirEntry*) ArchMemory::getIdentAddressOfPPN(pd);\
  uint32 pde_vpn = (vpage % (PAGE_TABLE_ENTRIES * PAGE_DIRECTORY_ENTRIES)) / PAGE_TABLE_ENTRIES;\
  uint32 pte_vpn = (vpage % (PAGE_TABLE_ENTRIES * PAGE_DIRECTORY_ENTRIES)) % PAGE_TABLE_ENTRIES;

extern PageDirEntry kernel_page_directory[PAGE_DIRECTORY_ENTRIES];
extern PageTableEntry kernel_page_tables[4 * PAGE_TABLE_ENTRIES];

union VAddr
{
        size_t addr;
        struct
        {
                size_t offset :12;
                size_t pti    :10;
                size_t pdi    :10;
        };
};

class ArchMemoryMapping
{
public:
        PageDirEntry* pd;
        PageTableEntry* pt;
        pointer page;

        ppn_t pd_ppn;
        ppn_t pt_ppn;
        ppn_t page_ppn;

        size_t page_size;

        size_t pdi;
        size_t pti;
};

class ArchMemory
{
public:
  ArchMemory();
  ArchMemory(ppn_t page_dir_ppn);
  ~ArchMemory();


  [[nodiscard]]
  bool mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access);
  void unmapPage(vpn_t virtual_page);

  [[nodiscard]]
  static bool mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages = false, bool memory_mapped_io = false);
  static void unmapKernelPage(vpn_t virtual_page, bool free_page = true);

  void printMappedPages();
  static void printMappedPages(uint32 page_dir_page);

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
  static pointer getIdentAddress(size_t address);

/**
 * Checks if a given Virtual Address is valid and is mapped to physical memory
 * @param pd page directory ppn
 * @param vaddress_to_check Virtual Address we want to check
 * @return physical address if the virtual address is mapped, zero otherwise
 */
  pointer checkAddressValid(size_t vaddress_to_check) const;
  static pointer checkAddressValid(ppn_t pd, size_t vaddress_to_check);

  const ArchMemoryMapping resolveMapping(vpn_t vpage) const;
  static const ArchMemoryMapping resolveMapping(ppn_t pd, vpn_t vpage);

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
  static uint32 get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, uint32* physical_page, uint32* physical_pte_page=0);

  static PageDirEntry* getKernelPagingStructureRootVirt();
  static size_t getKernelPagingStructureRootPhys();
  static void loadPagingStructureRoot(size_t cr3_value);

  static void flushLocalTranslationCaches(size_t addr);
  static void flushAllTranslationCaches(size_t addr);

  static void initKernelArchMem();

private:
  ArchMemory &operator=(ArchMemory const &src) = delete; // should never be implemented

  template<typename T, size_t NUM_ENTRIES>
  static bool tableEmpty(T* table);

  template<typename T>
  static void removeEntry(T* map, size_t index);

  template<typename T>
  void insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable);


  /**
   * ppn of the page dir page
   */
  ppn_t page_dir_page_;
};

extern ArchMemory kernel_arch_mem;
