//----------------------------------------------------------------------
//  $Id: ArchMemory.h,v 1.2 2005/05/19 15:43:42 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.h,v $
//  Revision 1.1  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//----------------------------------------------------------------------


#ifndef _ARCH_MEMORY_H_
#define _ARCH_MEMORY_H_


#include "types.h"


class ArchMemory
{
public:
  ArchMemory *instance() {return instance_;}
  static uint32 initNewPageDirectory(uint32 physicalPageToUse);
  static uint32 mapPage(uint32 pageDirectoryPage, uint32 virtualPage, uint32 physicalPage, uint32 userAccess);

private:
  ArchMemory *instance_;
};








#endif
