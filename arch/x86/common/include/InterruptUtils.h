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

typedef struct {
  uint32  number;      // handler number
  void (*offset)();    // pointer to handler function
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct {
    uint16 limit;
    size_t base;
} __attribute__((__packed__)) IDTR ;


class InterruptUtils
{
public:

  /**
   * initialises all items of the interrupthandlers
   */
  static void initialise();

  /**
   *
   */
  static void lidt(IDTR *idtr);

  /**
   *
   */
  static void countPageFault(uint64 address);

private:
  static InterruptHandlers handlers[];
  static uint64 pf_address;
  static uint64 pf_address_counter;
};

#endif
