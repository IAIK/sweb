#include "8259.h"
#include "ports.h"
#include "debug.h"
#include "ArchMulticore.h"

size_t PIC8259::outstanding_EOIs_ = 0;

uint32 PIC8259::cached_mask = 0xFFFF;

#define PIC_ICW1_INIT   0x11
#define PIC_ICW2_OFFSET 0x20
#define PIC_ICW4_8086   0x01

#define PIC_EOI 0x20

void PIC8259::initialise8259s()
{
  outportb(PIC_1_CONTROL_PORT, PIC_ICW1_INIT); /* ICW1 */
  outportb(PIC_1_DATA_PORT, PIC_ICW2_OFFSET); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
  outportb(PIC_1_DATA_PORT, 0x04); /* ICW3 */
  outportb(PIC_1_DATA_PORT, PIC_ICW4_8086); /* PIC_ICW4 */

  outportb(PIC_2_CONTROL_PORT, PIC_ICW1_INIT);
  outportb(PIC_2_DATA_PORT, PIC_ICW2_OFFSET + 0x8); /* ...IRQs 8...15 to INTs 28h...2Fh */
  outportb(PIC_2_DATA_PORT, 0x02);
  outportb(PIC_2_DATA_PORT, PIC_ICW4_8086);

  uint16 i;
  for (i=0;i<16;++i)
    disableIRQ(i);

  for (i=0;i<16;++i)
    sendEOI(i);

  outstanding_EOIs_ = 0;
}

void PIC8259::enableIRQ(uint16 number)
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

void PIC8259::disableIRQ(uint16 number)
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

void PIC8259::sendEOI(uint16 number)
{
  --outstanding_EOIs_;
  debug(A_INTERRUPTS, "CPU %zu, PIC8258::sendEOI %u, outstanding: %zu\n", ArchMulticore::getCpuID(), number, outstanding_EOIs_);
  assert(ArchMulticore::getCpuID() == 0);
  if (number > 7)
    outportb(PIC_2_CONTROL_PORT, PIC_EOI);

  outportb(PIC_1_CONTROL_PORT, PIC_EOI);
}
