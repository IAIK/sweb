/**
 * @file ArchInterrupts.h
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

#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#define IO_TIMEOUT (400000)

#include "types.h"

/** @class ArchInterrupts
 *
 * Collection of architecture dependant functions conerning Interrupts
 *
 */
class ArchInterrupts
{
public:

  /**
   * the timer handler callback will return 0 if nothing happened
   * or 1 if it wants to have a task switch
   * details of the task switch have to be set through other methods
   */
  typedef uint32 (*TimerHandlerCallback)();

  /**
   * gives the arch a chance to set things up the way it wants to
   *
   */
  static void initialise();

  /**
   * enables the Timer IRQ (0)
   *
   */
  static void enableTimer();

  /**
   * disables the Timer IRQ (0)
   *
   */
  static void disableTimer();

  /**
   * enables the Keyboard IRQ (1)
   *
   */
  static void enableKBD();

  /**
   * disables the Keyboard IRQ (1)
   *
   */
  static void disableKBD();

  /**
   * Signals EOI to the Interrupt-Controller, so the Controller
   * can resume sending us Interrupts
   *
   * @param number of Interrupt that finished, so we know which Interrupt-Controller
   * to signal
   *
   */
  static void EndOfInterrupt(uint16 number);

  /**
   * enable interrupts, no more lazy linear code execution time ;-)
   * (using sti on x86)
   */
  static void enableInterrupts();

  /**
   *
   * disables Interrupts (by using cli (clear interrupts) on x86)
   * @return bool true if Interrupts were enabled, false otherwise
   */
  static bool disableInterrupts();

  /**
   * on x86: tests if the IF Flag in EFLAGS is set, aka if the Interrupts are enabled
   *
   * @return bool true if Interrupts are enabled, false otherwise
   */
  static bool testIFSet();

  /**
   * yields if the IF Flag is set, else does hlt
   */
  static void yieldIfIFSet();

  static bool enabled;
};

#endif
