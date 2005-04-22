//----------------------------------------------------------------------
//   $Id: arch_interrupts.c,v 1.2 2005/04/22 19:43:04 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_interrupts.c,v $
//  Revision 1.1  2005/04/20 15:26:35  nomenquis
//  more and more stuff actually works
//
//----------------------------------------------------------------------

#include "arch_interrupts.h"
#include "arch_panic.h"

typedef struct
{
   /* pushed by pusha */
   uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
   
   /* pushed separately */
   uint32 ds, es, fs, gs;
   uint32 which_int, err_code;
   
   /* pushed by exception. Exception may also push err_code.
   user_esp and user_ss are pushed only if a privilege change occurs. */
   uint32 eip, cs, eflags, user_esp, user_ss;
} regs_t;


static generic_interrupt_handler* interrupt_vector_table[256];

void arch_handleInterrupt(void *r)
{
  regs_t *regs = (regs_t*)r;
  uint32 number = regs->which_int;
  if (number < 256 && interrupt_vector_table[number])
  {
    interrupt_vector_table[number]();
  }
  else
  {
    arch_panic("PANIC: No handler for this irq");
  }
}

void registerInterruptHandler(generic_interrupt_handler *handler, uint32 interrupt)
{
}
void unRegisterInterruptHandler(uint32 interrupt)
{
}

void initInterruptHandlers()
{
  uint32 i;
  for (i=0;i<256;++i)
    interrupt_vector_table[i] = 0;
}
