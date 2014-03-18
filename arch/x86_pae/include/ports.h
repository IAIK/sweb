/**
 * @file ports.h
 *
 */


#ifndef _PORTS_H_
#define _PORTS_H_

#include "types.h"


/**
 * reads 1 byte from the selected I/O port
 * @param port the I/O port number which is read
 *
 */
static inline uint8 inportb(uint16 port)
{
   uint8 return_val;
   __asm__ __volatile__ (
   "inb %1,%0"
   : "=a"(return_val)
   : "d"(port));

   return return_val;
}

/**
 * sends 1 byte of data to the specified I/O port
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 *
 */
static inline void outportb(uint16 port, uint8 val)
{
   __asm__ __volatile__ (
   "outb %b0, %w1"
   :
   : "a"(val), "d"(port));
}

#endif
