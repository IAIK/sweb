//----------------------------------------------------------------------
//   $Id: ArchCommon.h,v 1.1 2005/04/22 17:21:38 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
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
  static uint8 *getVESAConsoleLFBPtr();
  
};







#endif
