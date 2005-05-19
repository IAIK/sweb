//----------------------------------------------------------------------
//  $Id: ArchMemory.h,v 1.3 2005/05/19 19:35:30 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.h,v $
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
  pointer initNewPageDirectory(uint32 physicalPageToUse);
  static uint32 mapPage(uint32 pageDirectoryPage, uint32 virtualPage, uint32 physicalPage, uint32 userAccess);
  void insertPTE(page_directory_entry *whichPageDirectory, uint32 pageDirectoryPage, pointer pageTable);
  
  pointer get3GBAdressOfPPN(uint32 ppn)
  {
    return (3U*1024U*1024U*1024U) + (ppn * PAGE_SIZE);
  }
private:
};








#endif
