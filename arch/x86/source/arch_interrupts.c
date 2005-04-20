//----------------------------------------------------------------------
//   $Id: arch_interrupts.c,v 1.1 2005/04/20 15:26:35 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "arch_interrupts.h"
#include "arch_panic.h"

typedef struct
{
   /* pushed by pusha */
   unsigned edi, esi, ebp, esp, ebx, edx, ecx, eax;
   
   /* pushed separately */
   unsigned ds, es, fs, gs;
   unsigned which_int, err_code;
   
   /* pushed by exception. Exception may also push err_code.
   user_esp and user_ss are pushed only if a privilege change occurs. */
   unsigned eip, cs, eflags, user_esp, user_ss;
} regs_t;


static generic_interrupt_handler* interrupt_vector_table[256];

void arch_handleInterrupt(void *r)
{
  regs_t *regs = (regs_t*)r;
  int number = regs->which_int;
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
