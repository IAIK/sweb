//----------------------------------------------------------------------
//  $Id: ports.h,v 1.1 2005/07/31 18:31:09 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: ports.h,v $
//
//----------------------------------------------------------------------

#ifndef _PORTS_H_
#define _PORTS_H_

#include "types.h"

//  copied from x86...maybe allowed in xen but must be carefully not to use
//  any port already used by another domain...so disable it at the
//  moment for safety reasons

static inline uint8 inportb(uint16 port)
{
   uint8 return_val;
//    __asm__ __volatile__ (
//    "inb %1,%0"
//    : "=a"(return_val)
//    : "d"(port));
   
   return return_val;
}

static inline void outportb(uint16 port, uint8 val)
{
//    __asm__ __volatile__ (
//    "outb %b0, %w1"
//    :
//    : "a"(val), "d"(port));
}
#endif
