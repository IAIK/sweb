#pragma once

#define IO_TIMEOUT (400000)
#define IRQ0_TIMER_FREQUENCY 0

#include "types.h"

class ArchInterrupts
{
public:
  static void initialise();

  static void enableTimer();
  static void setTimerFrequency(uint32 freq);
  static void disableTimer();

  static void enableIRQ(uint16 num);
  static void disableIRQ(uint16 num);

  static void enableKBD();
  static void disableKBD();

  static void startOfInterrupt(uint16 number);

  /**
   * Signals EOI to the Interrupt-Controller, so the Controller
   * can resume sending us Interrupts
   *
   * @param number of Interrupt that finished, so we know which Interrupt-Controller
   * to signal
   *
   */
  static void endOfInterrupt(uint16 number);

  static void enableInterrupts();
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
};

class WithInterrupts
{
public:
    WithInterrupts(bool new_state)
    {
        previous_state_ = ArchInterrupts::testIFSet();
        if (new_state)
        {
            ArchInterrupts::enableInterrupts();
        }
        else
        {
            ArchInterrupts::disableInterrupts();
        }
    }

    ~WithInterrupts()
    {
        if(previous_state_)
        {
            ArchInterrupts::enableInterrupts();
        }
        else
        {
            ArchInterrupts::disableInterrupts();
        }
    }

    bool previousInterruptState()
    {
        return previous_state_;
    }

private:
    bool previous_state_;
};
