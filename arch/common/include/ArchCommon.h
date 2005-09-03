//----------------------------------------------------------------------
//   $Id: ArchCommon.h,v 1.8 2005/09/03 17:08:34 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.h,v $
//  Revision 1.7  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.6  2005/04/23 18:13:26  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
//  Revision 1.5  2005/04/23 15:58:31  nomenquis
//  lots of new stuff
//
//  Revision 1.4  2005/04/23 11:56:34  nomenquis
//  added interface for memory maps, it works now
//
//  Revision 1.3  2005/04/22 18:23:03  nomenquis
//  massive cleanups
//
//  Revision 1.2  2005/04/22 17:40:57  nomenquis
//  cleanup
//
//  Revision 1.1  2005/04/22 17:21:38  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#ifndef _ARCH_COMMON_H_
#define _ARCH_COMMON_H_

#include "types.h"

class ArchCommon
{
public:
  
  static uint32 haveVESAConsole(uint32 is_paging_set_up=1);
  static uint32 getVESAConsoleHeight();
  static uint32 getVESAConsoleWidth();
  static uint32 getVESAConsoleBitsPerPixel();
  static pointer getVESAConsoleLFBPtr(uint32 is_paging_set_up=1);
  
  static pointer getFBPtr(uint32 is_paging_set_up=1);

  static uint32 getNumUseableMemoryRegions();
  static uint32 getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type);

  static void memcpy(pointer dest, pointer src, size_t size);
  static void bzero(pointer s, size_t n, uint32 debug = 0);

  static uint32 getNumModules(uint32 is_paging_set_up=1);
  static uint32 getModuleStartAddress(uint32 num,uint32 is_paging_set_up=1);
  static uint32 getModuleEndAddress(uint32 num,uint32 is_paging_set_up=1);

  static void dummdumm(uint32 i, uint32 &used, uint32 &start, uint32 &end);

};







#endif
