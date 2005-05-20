//----------------------------------------------------------------------
//  $Id: ArchMemory.h,v 1.5 2005/05/20 11:58:10 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.h,v $
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

class ArchMemory
{
public:
  //ArchMemory *instance() {return instance_;}

  //creates a new Page Directory for a Process
  static void initNewPageDirectory(uint32 physical_page_to_use);
  //initialises a new PageTable located on physical_page_to_use
  static void initNewPageTable(uint32 physical_page_to_use);
  //links a pte into a pde
  static void insertPTE(uint32 physical_page_directory_page, uint32 pde_vpn, uint32 physical_page_table_page);
  //maps a physical page to a virtual page (pde and pte need to be set up first)
  static void mapPage(uint32 physical_page_directory_page, uint32 virtual_page, uint32 physical_page, uint32 user_access);
  //removes mapping to a virtual_page and returns ppn of that page
  static uint32 unmapPage(uint32 physical_page_directory_page, uint32 virtual_page);
  //unlinks a pte from a pde and returns the ppn where the pte is/was located
  static uint32 removePTE(uint32 physical_page_directory_page, uint32 pde_vpn);
  
  static pointer get3GBAdressOfPPN(uint32 ppn)
  {
    return (3U*1024U*1024U*1024U) + (ppn << PAGE_INDEX_OFFSET_BITS);
  }
private:
};








#endif
