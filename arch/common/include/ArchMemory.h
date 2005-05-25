//----------------------------------------------------------------------
//  $Id: ArchMemory.h,v 1.7 2005/05/25 08:27:48 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.h,v $
//  Revision 1.6  2005/05/20 14:07:20  btittelbach
//  Redesign everything
//
//  Revision 1.5  2005/05/20 11:58:10  btittelbach
//  much much nicer UserProcess Page Management, but still things to do
//
//  Revision 1.4  2005/05/19 20:04:16  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.3  2005/05/19 19:35:30  btittelbach
//  ein bisschen Arch Memory
//
//  Revision 1.2  2005/05/19 15:43:42  btittelbach
//  Ansätze für eine UserSpace Verwaltung
//
//  Revision 1.1  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//----------------------------------------------------------------------


#ifndef _ARCH_MEMORY_H_
#define _ARCH_MEMORY_H_


#include "types.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "panic.h"
#include "../../../common/include/mm/PageManager.h"

//Arch-VirtualMemoryUserSpaceObjekt
class ArchMemory
{
public:
  //creates a new Page Directory for a Process
  static void initNewPageDirectory(uint32 physical_page_to_use);
  //maps a physical page to a virtual page (pde and pte need to be set up first)
  static void mapPage(uint32 physical_page_directory_page, uint32 virtual_page, uint32 physical_page, uint32 user_access);
  //removes mapping to a virtual_page and returns ppn of that page
  static void unmapPage(uint32 physical_page_directory_page, uint32 virtual_page);
  //remove a PDE and all its Pages and PageTables
  static void freePageDirectory(uint32 physical_page_directory_page);

  static pointer physicalPageToKernelPointer(uint32 physical_page);


private:
  static void insertPTE(uint32 physical_page_directory_page, uint32 pde_vpn, uint32 physical_page_table_page);
  static void checkAndRemovePTE(uint32 physical_page_directory_page, uint32 pde_vpn);
  static pointer get3GBAdressOfPPN(uint32 ppn)
  {
    return (3U*1024U*1024U*1024U) + (ppn * PAGE_SIZE);
  }
};








#endif
