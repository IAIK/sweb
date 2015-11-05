/**
 * @file InterruptUtils.h
 *
 * Block devices update.
 * See BDRequest and BDManager on how to use this.
 * Currently ATADriver is functional. The driver tries to detect if IRQ
 * mode is available and adjusts the mode of operation. Currently PIO
 * modes with IRQ or without it are supported.
 *
 * Would be nice:
 * - add block PIO mode to read or write multiple sectors within one IRQ
 * - add DMA and UDMA mode :)
 *
 */


#ifndef _INTERRUPT_UTILS_H_
#define _INTERRUPT_UTILS_H_

#include "types.h"

static uint32 const NUM_INTERRUPT_HANDLERS = 256;

typedef struct {
  uint32  number;      // handler number
  void (*offset)();    // pointer to handler function
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct {
    uint16 limit;
    size_t base;
} __attribute__((__packed__)) IDTR ;

//---------------------------------------------------------------------------*/
struct GateDesc
{
  uint16 offset_low;       // low word of handler entry point's address
  uint16 segment_selector; // (code) segment the handler resides in
  uint8 reserved  : 5;     // reserved. set to zero
  uint8 zeros     : 3;     // set to zero
  uint8 type      : 3;     // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
  uint8 gate_size : 1;     // set to GATE_SIZE_16_BIT or GATE_SIZE_32_BIT
  uint8 unused    : 1;     // unsued - set to zero
  uint8 dpl       : 2;     // descriptor protection level
  uint8 present   : 1;     // present- flag - set to 1
  uint16 offset_high;      // high word of handler entry point's address
}__attribute__((__packed__));
//---------------------------------------------------------------------------*/


class InterruptUtils
{
public:

  /**
   * initialises all items of the interrupthandlers
   */
  static void initialise();

  static void initialise_ap();

  /**
   * not implemented
   * function from ArchInterrupts is used instead
   */
  static void enableInterrupts();

  /**
   * not implemented
   * function from ArchInterrupts is used instead
   */
  static void disableInterrupts();

  /**
   *
   */
  static void lidt(IDTR *idtr);

  /**
   *
   */
  static void countPageFault(uint64 address);

  static GateDesc* interrupt_gates;

private:
  static InterruptHandlers handlers[NUM_INTERRUPT_HANDLERS];
  static uint64 pf_address;
  static uint64 pf_address_counter;
};

#endif
