/**
 * @file InterruptUtils.h
 *
 * Block devices update.
 * See BDRequest and BDManager on how to use this.
 * Currently ATADriver is functional. The driver tries to detect if IRQ
 * mode is available and adjusts the mode of operation. Currently PIO
 * modes with IRQ or without it are supported.
 *
 * TODO:
 * - add block PIO mode to read or write multiple sectors within one IRQ
 * - add DMA and UDMA mode :)
 *
 */


#ifndef _INTERRUPT_UTILS_H_
#define _INTERRUPT_UTILS_H_

#include "types.h"

static uint32 const NUM_INTERRUPT_HANDLERS = 256;

typedef struct {
  uint32  number;       // handler number
  void (*handler)();    // pointer to handler function
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct {
    uint16 limit;
    uint32 base;
} __attribute__((__packed__)) IDTR ;


class InterruptUtils
{
public:

  /**
   * initialises all items of the interrupthandlers
   */
  static void initialise();

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

private:
  static InterruptHandlers handlers[NUM_INTERRUPT_HANDLERS];
};

#endif
