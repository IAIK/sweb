//----------------------------------------------------------------------
//  $Id: 8259.cpp,v 1.2 2005/04/24 10:32:05 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: 8259.cpp,v $
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
//----------------------------------------------------------------------

#include "8259.h"
#include "ports.h"

#define PIC_1_CONTROL_PORT 0x20
#define PIC_2_CONTROL_PORT 0xA0
#define PIC_1_DATA_PORT 0x21
#define PIC_2_DATA_PORT 0xA1

static uint32 cached_mask = 0xFFFF;

void initialise8259s()
{
	outportb(PIC_1_CONTROL_PORT, 0x11); /* ICW1 */
	outportb(PIC_2_CONTROL_PORT, 0x11);

	outportb(PIC_1_DATA_PORT, 0x20); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
	outportb(PIC_2_DATA_PORT, 0x28); /* ...IRQs 8...15 to INTs 28h...2Fh */

	outportb(PIC_1_DATA_PORT, 0x04); /* ICW3 */
	outportb(PIC_2_DATA_PORT, 0x02);

	outportb(PIC_1_DATA_PORT, 0x01); /* ICW4 */
	outportb(PIC_2_DATA_PORT, 0x01);

  uint32 i;
  for (i=0;i<32;++i)
    disableIRQ(i);
}

void disableIRQ(uint16 number)
{
   uint32 mask = 1 << number;
   cached_mask |= mask;
   if (number & 8)
   {
      outportb(PIC_2_DATA_PORT,((cached_mask/8)%8));
   }
   else
   {
      outportb(PIC_1_DATA_PORT,(cached_mask%8));
   }
}


void enableIRQ(uint16 number)
{
   uint32 mask = 1 << number;
   cached_mask &= ~mask;
   if (number & 8)
   {
      outportb(PIC_2_DATA_PORT,((cached_mask/8)%8));
   }
   else
   {
      outportb(PIC_1_DATA_PORT,(cached_mask%8));
   }
}
