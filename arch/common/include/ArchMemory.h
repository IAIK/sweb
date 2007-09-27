/**
 * @file ArchMemory.h
 *
 */

#ifndef _ARCH_MEMORY_H_
#define _ARCH_MEMORY_H_

#include "types.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "panic.h"
#include "mm/PageManager.h"

//Arch-VirtualMemoryUserSpaceObjekt
/**
 *
 * Collection of architecture dependant functions concerning Memory and Pages
 *
 */
class ArchMemory
{
public:

/** 
 *
 * creates a new Page-Directory for a UserProccess by copying the
 * Kernel-Page-Directory
 *
 * @param physical_page_to_use where the new PDE should be
 */
  static void initNewPageDirectory(uint32 physical_page_to_use);

/** 
 *
 * maps a virtual page to a physical page (pde and pte need to be set up first)
 *
 * @param physical_page_directory_page Real Page where the PDE to work on resides
 * @param virtual_page 
 * @param physical_page
 * @param user_access PTE User/Supervisor Flag, governing the binary Paging
 * Privilege Mechanism
 * @param page_size Optional, defaults to 4k pages, but you ned to set it to
 * 1024*4096 if you want to map a 4m page
 */
  static void mapPage(uint32 physical_page_directory_page, uint32 virtual_page, uint32 physical_page, uint32 user_access, uint32 page_size=PAGE_SIZE);

/**
 * removes the mapping to a virtual_page by marking its PTE Entry as non valid
 *
 * @param physical_page_directory_page Real Page where the PDE to work on resides
 * @param virtual_page which will be invalidated
 */
  static void unmapPage(uint32 physical_page_directory_page, uint32 virtual_page);

/**
 * recursively remove a PageDirectoryEntry and all its Pages and PageTables
 *
 * @param physical_page_directory_page of PDE to remove
 */
  static void freePageDirectory(uint32 physical_page_directory_page);

//  static pointer physicalPageToKernelPointer(uint32 physical_page);

/**
 * Takes a Physical Page Number in Real Memory and returns a virtual address than
 * can be used to access given page
 * @param ppn Physical Page Number
 * @param page_size Optional, defaults to 4k pages, but you ned to set it to
 * 1024*4096 if you have a 4m page number
 * @return Virtual Address above 3GB pointing to the start of a memory segment that
 * is mapped to the physical page given
 */
  static pointer get3GBAdressOfPPN(uint32 ppn, uint32 page_size=PAGE_SIZE)
  {
    return (3U*1024U*1024U*1024U) + (ppn * page_size);
  }

/**
 * Checks if a given Virtual Address is valid and is mapped to real memory
 * @param physical_page_directory_page Real Page where the PDE can be found
 * @param vaddress_to_check Virtual Address we want to check
 * @return true: if mapping exists\nfalse: if the given virtual address is unmapped
 * and accessing it would result in a pageFault
 */
  static bool checkAddressValid(uint32 physical_page_directory_page, uint32 vaddress_to_check);

/**
 * Takes a virtual_page and search through the pageTable and pageDirectory for the
 * physical_page it refers to
 * to get a physical address (which you can only use by adding 3gb to it) multiply
 * the &physical_page with the return value
 * @param virtual_page virtual Page to look up
 * @param &physical_pag Reference to the result
 * @return 0: if the virtual page doesn't map to any physical page\notherwise
 * returns the page size in byte (4096 for 4k pages or 4096*1024 for 4m pages)
 */
  static uint32 getPhysicalPageOfVirtualPageInKernelMapping(uint32 virtual_page, uint32 *physical_page);

private:

/** 
 * adds a PageTableEntry to the given PageDirectory
 * @param physical_page_directory_page Real Page where the PDE we want to ad a PTE
 * to, resides
 * @param pde_vpn The Virtual Page Number inside the PDE that shall point to our new
 * PTE
 * @param physical_page_table_page Real Page where the PTE we want to add, resides
 */
  static void insertPTE(uint32 physical_page_directory_page, uint32 pde_vpn, uint32 physical_page_table_page);

/**
 * Removes a PageTableEntry from a PageDiretory if it was there in the first place
 * @param physical_page_directory_page Real Page where the PDE is
 * @param pde_vpn Virtual Page Number inside the PDE of the PTE we want to remove
 */
  static void checkAndRemovePTE(uint32 physical_page_directory_page, uint32 pde_vpn);

};

#endif
