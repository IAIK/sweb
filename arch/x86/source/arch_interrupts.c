//----------------------------------------------------------------------
//   $Id: arch_interrupts.c,v 1.3 2005/04/24 10:32:05 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_interrupts.c,v $
//  Revision 1.2  2005/04/22 19:43:04  nomenquis
//   more poison added
//
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

#define FOOBAR(asd)       case asd##0:\
        arch_panic("PANIC: No handler for irq asd##0");\
      case asd##1:\
        arch_panic("PANIC: No handler for irq asd##1");\
      case asd##2:\
        arch_panic("PANIC: No handler for irq asd##2");\
      case asd##3:\
        arch_panic("PANIC: No handler for irq asd##3");\
      case asd##4:\
        arch_panic("PANIC: No handler for irq asd##4");\
      case asd##5:\
        arch_panic("PANIC: No handler for irq asd##5");\
      case asd##6:\
        arch_panic("PANIC: No handler for irq asd##6");\
      case asd##7:\
        arch_panic("PANIC: No handler for irq asd##7");\
      case asd##8:\
        arch_panic("PANIC: No handler for irq asd##8");\
      case asd##9:\
        arch_panic("PANIC: No handler for irq asd##9");\

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
    switch(number)
    {
      FOOBAR( )
      FOOBAR(1)
      FOOBAR(2)
      FOOBAR(3)
      FOOBAR(4)
      FOOBAR(5)
      FOOBAR(6)
      FOOBAR(7)
      FOOBAR(8)
      FOOBAR(9)
      FOOBAR(10)
    }
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
