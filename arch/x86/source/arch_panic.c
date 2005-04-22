//----------------------------------------------------------------------
//   $Id: arch_panic.c,v 1.2 2005/04/22 19:43:04 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_panic.c,v $
//  Revision 1.1  2005/04/20 15:26:35  nomenquis
//  more and more stuff actually works
//
//----------------------------------------------------------------------

#include "arch_panic.h"

void arch_panic(uint8 *mesg)
{
  uint8 * fb = (uint8*)0xC00B8000;
  uint32 i=0;
  while (mesg && *mesg)
  {
    fb[i++] = *mesg++;
    fb[i++] = 0x9f;
  }
  for (;;);
}
