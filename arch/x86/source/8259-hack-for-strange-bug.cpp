#include "ports.h"
#include "8259.h"

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
