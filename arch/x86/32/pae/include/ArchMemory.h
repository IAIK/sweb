#pragma once

#include "types.h"
#include "paging-definitions.h"

#define RESOLVEMAPPING(pdpt,vpage) ;\
  uint32 pdpte_vpn = vpage / (PAGE_TABLE_ENTRIES * PAGE_DIRECTORY_ENTRIES);\
  PageDirEntry* page_directory = (PageDirEntry*) ArchMemory::getIdentAddressOfPPN(pdpt[pdpte_vpn].page_ppn);\
  uint32 pde_vpn = (vpage % (PAGE_TABLE_ENTRIES * PAGE_DIRECTORY_ENTRIES)) / PAGE_TABLE_ENTRIES;\
  uint32 pte_vpn = (vpage % (PAGE_TABLE_ENTRIES * PAGE_DIRECTORY_ENTRIES)) % PAGE_TABLE_ENTRIES;

extern PageDirPointerTableEntry kernel_page_directory_pointer_table[PAGE_DIRECTORY_POINTER_TABLE_ENTRIES];
extern PageDirEntry kernel_page_directory[4 * PAGE_DIRECTORY_ENTRIES];
extern PageTableEntry kernel_page_tables[8 * PAGE_TABLE_ENTRIES];

class ArchMemoryMapping
{
public:
    PageDirPointerTableEntry* pdpt;
    PageDirEntry* pd;
    PageTableEntry* pt;
    pointer page;

    ppn_t pd_ppn;
    ppn_t pt_ppn;
    ppn_t page_ppn;

    size_t page_size;

    size_t pdpti;
    size_t pdi;
    size_t pti;
};

class ArchMemory
{
public:
  ArchMemory();
  ArchMemory(PageDirPointerTableEntry* pdpt_addr);

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
  __attribute__((warn_unused_result)) bool mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access);

/**
 * removes the mapping to a virtual_page by marking its PTE Entry as non valid
 *
 * @param physical_page_directory_page Real Page where the PDE to work on resides
 * @param virtual_page which will be invalidated
 */
  void unmapPage(vpn_t virtual_page);

  ~ArchMemory();

/**
 * recursively remove a PageDirectoryEntry and all its Pages and PageTables
 *
 * @param physical_page_directory_page of PDE to remove
 */
  void freePageDirectory(ppn_t physical_page_directory_page);

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
 * Checks if a given Virtual Address is valid and is mapped to real memory
 * @param pdpt page dir pointer table
 * @param vaddress_to_check Virtual Address we want to check
 * @return true: if mapping exists\nfalse: if the given virtual address is unmapped
 * and accessing it would result in a pageFault
 */
  pointer checkAddressValid(uint32 vaddress_to_check);
  static pointer checkAddressValid(PageDirPointerTableEntry* pdpt, uint32 vaddress_to_check);

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
  static uint32 get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, size_t* physical_page, uint32* physical_pte_page=0);

  const ArchMemoryMapping resolveMapping(vpn_t vpage);
  static const ArchMemoryMapping resolveMapping(PageDirPointerTableEntry* pdpt, vpn_t vpage);

/**
 *
 * maps a virtual page to a physical page in kernel mapping
 *
 * @param virtual_page
 * @param physical_page
 */
  static __attribute__((warn_unused_result)) bool mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages = false, bool memory_mapped_io = false);

/**
 * removes the mapping to a virtual_page by marking its PTE Entry as non valid
 * in kernel mapping
 *
 * @param virtual_page which will be invalidated
 */
  static void unmapKernelPage(vpn_t virtual_page, bool free_page = true);

  static void initKernelArchMem();

  size_t getPagingStructureRootPhys();
  uint32 getValueForCR3();

  static PageDirPointerTableEntry* getKernelPagingStructureRootVirt();
  static size_t getKernelPagingStructureRootPhys();
  static void loadPagingStructureRoot(size_t cr3_value);

  static void flushLocalTranslationCaches(size_t addr);
  static void flushAllTranslationCaches(size_t addr);

  static const size_t RESERVED_START = 0x80000ULL;
  static const size_t RESERVED_END = 0xC0000ULL;

private:
  ArchMemory &operator=(ArchMemory const &src) = delete; // should never be implemented

  template<typename T, size_t NUM_ENTRIES>
  static bool tableEmpty(T* table);

  template<typename T>
  void removeEntry(T* table, size_t index);

  template<typename T>
  void insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable);


  PageDirPointerTableEntry page_dir_pointer_table_space_[2 * PAGE_DIRECTORY_POINTER_TABLE_ENTRIES];
  // why 2* ? this is a hack because this table has to be aligned to its own
  // size 0x20... this way we allow to set an aligned pointer in the constructor.
  // but why cant we just use __attribute__((aligned(0x20))) ? i'm not sure...
  // i think because were in a class and the object will be allocated by the
  // KMM which will just return some not-aligned block, thus this aligned block
  // gets not-aligned in memory -- DG

  PageDirPointerTableEntry* page_dir_pointer_table_;
};

extern ArchMemory kernel_arch_mem;
