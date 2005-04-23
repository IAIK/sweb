//----------------------------------------------------------------------
//  $Id: ports.h,v 1.1 2005/04/23 20:08:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef _PORTS_H_
#define _PORTS_H_

#include "types.h"

static inline uint8 inportb(uint16 port)
{
   uint8 return_val;
   __asm__ __volatile__ (
   "inb %1,%0"
   : "=a"(return_val)
   : "d"(port));
   
   return return_val;
}

static inline void outportb(uint16 port, uint8 val)
{
   __asm__ __volatile__ (
   "outb %b0, %w1"
   :
   : "a"(val), "d"(port));
}
#endif
