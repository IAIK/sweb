//----------------------------------------------------------------------
//   $Id: arch_interrupts.h,v 1.1 2005/04/20 15:26:35 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "types.h"

void arch_handleInterrupt(void *regs);

typedef uint32 (*generic_interrupt_handler());

void registerInterruptHandler(generic_interrupt_handler *handler, uint32 interrupt);
void unRegisterInterruptHandler(uint32 interrupt);
