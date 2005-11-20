//----------------------------------------------------------------------
//  $Id: 8259.cpp,v 1.7 2005/11/20 21:18:08 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: 8259.cpp,v $
//  Revision 1.6  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//  Revision 1.5  2005/04/25 23:09:18  nomenquis
//  fubar 2
//
//  Revision 1.4  2005/04/25 22:41:58  nomenquis
//  foobar
//
//  Revision 1.3  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.2  2005/04/24 10:32:05  nomenquis
//  better debugging
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
//----------------------------------------------------------------------

#include "8259.h"
#include "ports.h"


uint32 cached_mask = 0xFFFF;

void initialise8259s()
{
	outportb(PIC_1_CONTROL_PORT, 0x11); /* ICW1 */
	outportb(PIC_1_DATA_PORT, 0x20); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
	outportb(PIC_1_DATA_PORT, 0x04); /* ICW3 */
	outportb(PIC_1_DATA_PORT, 0x01); /* ICW4 */
	
	outportb(PIC_2_CONTROL_PORT, 0x11);
	outportb(PIC_2_DATA_PORT, 0x28); /* ...IRQs 8...15 to INTs 28h...2Fh */
	outportb(PIC_2_DATA_PORT, 0x02);
	outportb(PIC_2_DATA_PORT, 0x01);

  uint16 i;
  for (i=0;i<16;++i)
    disableIRQ(i);
  
  for (i=0;i<16;++i)
    sendEOI(i);

}
void enableIRQ(uint16 number)
{
   uint32 mask = 1 << number;
   cached_mask &= ~mask;
   if (number & 8)
   {
      outportb(PIC_2_DATA_PORT,((cached_mask>>8)));
   }
   else
   {
      outportb(PIC_1_DATA_PORT,(cached_mask%8));
   }
}


void disableIRQ(uint16 number)
{
   uint32 mask = 1 << number;
   cached_mask |= mask;
   if (number & 8)
   {
      outportb(PIC_2_DATA_PORT,((cached_mask>>8)));
   }
   else
   {
      outportb(PIC_1_DATA_PORT,(cached_mask%8));
   }
}

void sendEOI(uint16 number)
{
  if (number > 7)
    outportb(PIC_2_CONTROL_PORT,0x20);

  outportb(PIC_1_CONTROL_PORT,0x20);
}
