#include "8259.h"
#include "ports.h"
#include "debug.h"
#include "ArchMulticore.h"
#include "assert.h"


bool PIC8259::exists = true;
size_t PIC8259::outstanding_EOIs_ = 0;

uint16 PIC8259::cached_mask = 0xFFFF;

#define PIC_ICW1_INIT   0x11
#define PIC_ICW2_OFFSET 0x20 // offset for IRQ -> interrupt vector mapping (i.e. IRQ 0 is mapped to vector 32)
#define PIC_ICW4_8086   0x01

#define PIC_EOI 0x20

void PIC8259::initialise8259s()
{
  debug(PIC_8259, "Init 8259 Programmable Interrupt Controller\n");
  outportb(PIC_1_CONTROL_PORT, PIC_ICW1_INIT); /* ICW1 */
  outportb(PIC_1_DATA_PORT, PIC_ICW2_OFFSET); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
  outportb(PIC_1_DATA_PORT, 0x04); /* ICW3 */
  outportb(PIC_1_DATA_PORT, PIC_ICW4_8086); /* PIC_ICW4 */

  outportb(PIC_2_CONTROL_PORT, PIC_ICW1_INIT);
  outportb(PIC_2_DATA_PORT, PIC_ICW2_OFFSET + 0x8); /* ...IRQs 8...15 to INTs 28h...2Fh */
  outportb(PIC_2_DATA_PORT, 0x02);
  outportb(PIC_2_DATA_PORT, PIC_ICW4_8086);

  // Mask all interrupts
  setIrqMask(0xFFFF);

  for (size_t i=0;i<16;++i)
    sendEOI(i);

  outstanding_EOIs_ = 0;
}

bool PIC8259::isIRQEnabled(uint16 number)
{
  assert(number < 16);
  return !(cached_mask & (1 << number));
}

void PIC8259::enableIRQ(uint16 number)
{
  debug(PIC_8259, "PIC8259, enable IRQ %x\n", number);
  assert(number < 16);
  cached_mask &= ~(1 << number);
  if (number >= 8)
  {
    outportb(PIC_2_DATA_PORT, cached_mask >> 8);
  }
  else
  {
    outportb(PIC_1_DATA_PORT, cached_mask % 8);
  }

  if(!isIRQEnabled(2) && (number >= 8))
  {
    enableIRQ(2); // Enable slave cascade
  }
}

void PIC8259::disableIRQ(uint16 number)
{
  debug(PIC_8259, "PIC8259, disable IRQ %x\n", number);
  assert(number < 16);
  cached_mask |= (1 << number);
  if (number >= 8)
  {
    outportb(PIC_2_DATA_PORT, cached_mask >> 8);
  }
  else
  {
    outportb(PIC_1_DATA_PORT, cached_mask & 0xFF);
  }

  if(((cached_mask & 0xFF00) == 0xFF00) && isIRQEnabled(2) && (number >= 8))
  {
    disableIRQ(2); // Disable slave cascade
  }
}

void PIC8259::setIrqMask(uint16 mask)
{
  debug(PIC_8259, "PIC8259, set IRQ mask %x\n", mask);
  cached_mask = mask;
  outportb(PIC_2_DATA_PORT, cached_mask >> 8);
  outportb(PIC_1_DATA_PORT, cached_mask & 0xFF);
}

void PIC8259::sendEOI(uint16 number)
{
  if(PIC_8259 & OUTPUT_ADVANCED)
      debug(PIC_8259, "PIC8259, send EOI for IRQ %x\n", number);

  assert(number <= 16);
  --outstanding_EOIs_;

  if (number >= 8)
  {
    outportb(PIC_2_CONTROL_PORT, PIC_EOI);
  }

  outportb(PIC_1_CONTROL_PORT, PIC_EOI);
}
