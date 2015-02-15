#ifdef _DUMMY_HANDLERS_H_
#error This file may not be included more than once
#else
#define _ERROR_HANDLERS_H_

char const *intel_manual =
                  "See Intel 64 and IA-32 Architectures Software Developer's Manual\n"
                  "Volume 3A: System Programming Guide\n"
                  "for further information on what happened\n\n";

ERROR_HANDLER(0,#DE: Divide by Zero)
ERROR_HANDLER(4,#OF: Overflow (INTO Instruction))
ERROR_HANDLER(5,#BR: Bound Range Exceeded)
ERROR_HANDLER(6,#OP: Invalid OP Code)
ERROR_HANDLER(7,#NM: FPU Not Avaiable or Ready)
ERROR_HANDLER(8,#DF: Double Fault)
ERROR_HANDLER(9,#MF: FPU Segment Overrun)
ERROR_HANDLER(10,#TS: Invalid Task State Segment (TSS))
ERROR_HANDLER(11,#NP: Segment Not Present (WTF ?))
ERROR_HANDLER(12,#SS: Stack Segment Fault)
ERROR_HANDLER(13,#GF: General Protection Fault (unallowed memory reference) )
ERROR_HANDLER(16,#MF: Floting Point Error)
ERROR_HANDLER(17,#AC: Alignment Error (Unaligned Memory Reference))
ERROR_HANDLER(18,#MC: Machine Check Error)
ERROR_HANDLER(19,#XF: SIMD Floting Point Error)

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

#define IRQ_HANDLER(x) extern "C" void arch_irqHandler_##x(); \
  extern "C" void irqHandler_##x ()  {  \
    kprintfd("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    kprintf("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    ArchInterrupts::EndOfInterrupt(x); \
  }; \

IRQ_HANDLER(2)
IRQ_HANDLER(5)
IRQ_HANDLER(7)
IRQ_HANDLER(8)
IRQ_HANDLER(10)
IRQ_HANDLER(12)
IRQ_HANDLER(13)

extern "C" void arch_dummyHandler();

#define DUMMYHANDLER(X) {X, &arch_dummyHandler},
#define ERRORHANDLER(X) {X, &arch_errorHandler_##X},
#define IRQHANDLER(X) {X + 32, &arch_irqHandler_##X},
InterruptHandlers InterruptUtils::handlers[] = {
  ERRORHANDLER(0)
  ERRORHANDLER(4)
  ERRORHANDLER(5)
  ERRORHANDLER(6)
  ERRORHANDLER(7)
  ERRORHANDLER(8)
  ERRORHANDLER(9)
  ERRORHANDLER(10)
  ERRORHANDLER(11)
  ERRORHANDLER(12)
  ERRORHANDLER(13)
  {14, &arch_pageFaultHandler},
  ERRORHANDLER(16)
  ERRORHANDLER(17)
  ERRORHANDLER(18)
  ERRORHANDLER(19)
  IRQHANDLER(0)
  IRQHANDLER(1)
  IRQHANDLER(2)
  IRQHANDLER(3)
  IRQHANDLER(4)
  IRQHANDLER(5)
  IRQHANDLER(6)
  IRQHANDLER(7)
  IRQHANDLER(8)
  IRQHANDLER(9)
  IRQHANDLER(10)
  IRQHANDLER(11)
  IRQHANDLER(12)
  IRQHANDLER(13)
  IRQHANDLER(14)
  IRQHANDLER(15)
  {65, &arch_irqHandler_65},
  {128, &arch_syscallHandler},
  {0,0}
};

#endif
