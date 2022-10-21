#pragma once

#include "types.h"
#include "ports.h"

class PIC8259
{
public:
/**
 * sends the initialisation and operational command words to CPU
 *
 */
  static void initialise8259s();

  static uint16 cached_mask;

/**
 * enables the interrupt Request with the given number 0 to 15
 *
 */
  static void enableIRQ(uint16 number);

/**
 * disables the interrupt Request with the given number 0 to 15
 *
 */
  static void disableIRQ(uint16 number);

  static void setIrqMask(uint16 mask);

  static bool isIRQEnabled(uint16 number);

/**
 * sends the EOI signal to a Programmable Interrupt Controller (PIC)
 * to indicate the completion of interrupt processing for the given
 * interrupt.
 *
 */
  static void sendEOI(uint16 number);

  static size_t outstanding_EOIs_;

private:
  enum
  {
          PIC_1_CONTROL_PORT = 0x20,
          PIC_2_CONTROL_PORT = 0xA0,
          PIC_1_DATA_PORT    = 0x21,
          PIC_2_DATA_PORT    = 0xA1,
  };
};
