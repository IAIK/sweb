//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.3 2005/04/24 10:06:08 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.2  2005/04/23 20:32:30  nomenquis
//  timer interrupt works
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
//----------------------------------------------------------------------

#include "ArchInterrupts.h"
#include "8259.h"
#include "structures.h"
#include "ports.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;
static ArchInterrupts::TimerHandlerCallback timer_handler_callback;

static void setInterruptVector(uint32 key, pointer handler_function);
static void handleTimerInterrupt(regs_t *regs);


// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{
  initialise8259s();
  setInterruptVector(0x20,(pointer)&handleTimerInterrupt);
}

void ArchInterrupts::setTimerHandler(TimerHandlerCallback timer_handler)
{
  timer_handler_callback = timer_handler;
}

void ArchInterrupts::enableTimer()
{
  enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
  disableIRQ(0);
}

void ArchInterrupts::enableInterrupts()
{
     __asm__ __volatile__("sti"
   :
   :
   );

}

void ArchInterrupts::disableInterrupts()
{
   uint32 ret_val;

 __asm__ __volatile__("pushfl\n"
                      "popl %0\n"
                      "cli"
 : "=a"(ret_val)
 :);
 
// return ret_val;

}


extern "C" void setvect(vector_t *v, uint32 vect_num);

void setInterruptVector(uint32 key, pointer handler_function)
{

  vector_t int_vect;
  int_vect.access_byte = 0x8E;
  int_vect.eip = handler_function;
  setvect(&int_vect, key);
}

typedef struct ArchThreadInfo
{
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  dpl;       // 64
  uint32  esp0;      // 68
  uint32  ss0;       // 72
  uint32  cr3;       // 76
  uint32  fpu[27];   // 80
};

typedef struct ArchThread
{
  ArchThreadInfo *thread_info;
  uint8 *stack;
  
};
extern ArchThread *thread1;
extern ArchThread *thread2;
extern ArchThread *current;
extern "C" void arch_switch_thread1();

void handleTimerInterrupt(regs_t *regs)
{
  if (timer_handler_callback)
    timer_handler_callback();
  
  outportb(0x20, 0x20);
//  arch_switch_thread1();
}
