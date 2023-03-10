#pragma once

#define IO_TIMEOUT (4000000)
#define IRQ0_TIMER_FREQUENCY 0

#include "types.h"
#include "IrqDomain.h"
#include "ArchCpuLocalStorage.h"

extern cpu_local IrqDomain* cpu_root_irq_domain_;
extern cpu_local IrqDomain cpu_irq_vector_domain_;

class ArchInterrupts
{
public:
  static void initialise();

  static void enableTimer();
  static void setTimerFrequency(uint32 freq);
  static void disableTimer();

  static void enableIRQ(uint16 num, bool enable = true);
  static void disableIRQ(uint16 num);

  static void enableIRQ(const IrqDomain::DomainIrqHandle& domain_irq, bool enable = true);
  static void disableIRQ(const IrqDomain::DomainIrqHandle& domain_irq);

  static IrqDomain& currentCpuRootIrqDomain();
  /**
     IRQ domain for ISA interrupts (PIT, mouse, keyboard, disk, ...)
   */
  static IrqDomain& isaIrqDomain();

  static void enableKBD();
  static void disableKBD();


  static void startOfInterrupt(uint16 number);
  static void startOfInterrupt(const IrqDomain::DomainIrqHandle& irq_handle);

  /**
   * Signals EOI to the Interrupt-Controller, so the Controller
   * can resume sending us Interrupts
   *
   * @param number of Interrupt that finished, so we know which Interrupt-Controller
   * to signal
   *
   */
  static void endOfInterrupt(uint16 number);
  static void endOfInterrupt(const IrqDomain::DomainIrqHandle& irq_handle);

  static void handleInterrupt(uint16_t irq);
  static void handleInterrupt(const IrqDomain::DomainIrqHandle& irq_handle);

  static void enableInterrupts();
  static bool disableInterrupts();
  static void setInterrupts(bool state)
  {
      if (state)
      {
          enableInterrupts();
      }
      else
      {
          disableInterrupts();
      }
  }

  /**
   * on x86: tests if the IF Flag in EFLAGS is set, aka if the Interrupts are enabled
   *
   * @return bool true if Interrupts are enabled, false otherwise
   */
  static bool testIFSet();

  /**
   * yields if the IF Flag is set, else does nothing
   */
  static void yieldIfIFSet();
};

struct ArchThreadRegisters;
class Thread;
extern "C" [[noreturn]] void contextSwitch(Thread* target_thread = nullptr, ArchThreadRegisters* target_registers = nullptr);

class WithInterrupts
{
public:
    WithInterrupts(bool new_state)
    {
        previous_state_ = ArchInterrupts::testIFSet();
        ArchInterrupts::setInterrupts(new_state);
    }

    ~WithInterrupts()
    {
        ArchInterrupts::setInterrupts(previous_state_);
    }

    bool previousInterruptState()
    {
        return previous_state_;
    }

private:
    bool previous_state_;
};
