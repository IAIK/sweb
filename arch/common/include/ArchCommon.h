//----------------------------------------------------------------------
//   $Id: ArchCommon.h,v 1.2 2005/04/22 17:40:57 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.h,v $
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
  
  static uint32 haveVESAConsole();
  static uint32 getVESAConsoleHeight();
  static uint32 getVESAConsoleWidth();
  static uint32 getVESAConsoleBitsPerPixel();
  static uint32 getVESAConsoleLFBPtr();
  
};







#endif
