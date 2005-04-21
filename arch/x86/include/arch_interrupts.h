//----------------------------------------------------------------------
//   $Id: arch_interrupts.h,v 1.2 2005/04/21 21:31:24 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_interrupts.h,v $
//  Revision 1.1  2005/04/20 15:26:35  nomenquis
//  more and more stuff actually works
//
//----------------------------------------------------------------------

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif
  
void arch_handleInterrupt(void *regs);

typedef uint32 (*generic_interrupt_handler());

void registerInterruptHandler(generic_interrupt_handler *handler, uint32 interrupt);
void unRegisterInterruptHandler(uint32 interrupt);
  
#ifdef __cplusplus
}
#endif
