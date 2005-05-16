//----------------------------------------------------------------------
//  $Id: ArchMemory.h,v 1.1 2005/05/16 20:37:51 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------


#ifndef _ARCH_MEMORY_H_
#define _ARCH_MEMORY_H_


#include "types.h"


class ArchMemory
{
public:
  
  static uint32 initNewPageDirectory(uint32 physicalPageToUse);
  static uint32 mapPage(uint32 pageDirectoryPage, uint32 virtualPage, uint32 physicalPage, uint32 userAccess);

  
};








#endif
