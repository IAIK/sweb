/**
 * @file InterruptUtils.cpp
 *
 */

#include "InterruptUtils.h"
#include "new.h"
#include "arch_panic.h"
#include "ArchMemory.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "Console.h"
#include "FrameBufferConsole.h"
#include "Terminal.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "debug_bochs.h"

#include "arch_keyboard_manager.h"
#include "arch_bd_manager.h"
#include "panic.h"

#include "Thread.h"
#include "ArchInterrupts.h"
#include "backtrace.h"

//remove this later
#include "Thread.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"
//---------------------------------------------------------------------------*/

void InterruptUtils::initialise()
{
}

extern uint32* currentStack;
extern Console* main_console;

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

#define KEXP_USER_ENTRY \
  KEXP_TOP3 \
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp0));

#define KEXP_TOP3 \
  asm("sub lr, lr, #4"); \
  KEXP_TOPSWI

// parts of this code are taken from http://wiki.osdev.org/ARM_Integrator-CP_IRQTimerAndPIC
#define KEXP_TOPSWI \
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->pc));\
  asm("mov %[v], r0" : [v]"=r" (currentThreadInfo->r0));\
  asm("mov %[v], r1" : [v]"=r" (currentThreadInfo->r1));\
  asm("mov %[v], r2" : [v]"=r" (currentThreadInfo->r2));\
  asm("mov %[v], r3" : [v]"=r" (currentThreadInfo->r3));\
  asm("mov %[v], r4" : [v]"=r" (currentThreadInfo->r4));\
  asm("mov %[v], r5" : [v]"=r" (currentThreadInfo->r5));\
  asm("mov %[v], r6" : [v]"=r" (currentThreadInfo->r6));\
  asm("mov %[v], r7" : [v]"=r" (currentThreadInfo->r7));\
  asm("mov %[v], r8" : [v]"=r" (currentThreadInfo->r8));\
  asm("mov %[v], r9" : [v]"=r" (currentThreadInfo->r9));\
  asm("mov %[v], r10" : [v]"=r" (currentThreadInfo->r10));\
  asm("mov %[v], r11" : [v]"=r" (currentThreadInfo->r11));\
  asm("mov %[v], r12" : [v]"=r" (currentThreadInfo->r12));\
  asm("mrs r0, spsr"); \
  asm("mov %[v], r0" : [v]"=r" (currentThreadInfo->cpsr));\
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0x1f \n\
       orr r0, r0, #0x1f \n\
       msr cpsr, r0 \n\
      ");\
  asm("mov %[v], sp" : [v]"=r" (currentThreadInfo->sp));\
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->lr));

#define KEXP_BOTSWI \
    KEXP_BOT3

#define KEXP_BOT3 \
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp));\
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->lr));\
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0x1f \n\
       orr r0, r0, #0x13 \n\
       msr cpsr, r0 \n\
      ");\
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->cpsr));\
  asm("msr spsr, r0"); \
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp));\
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->lr));\
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->pc));\
  asm("push {r0}"); \
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->r0));\
  asm("mov r1, %[v]" : : [v]"r" (currentThreadInfo->r1));\
  asm("mov r2, %[v]" : : [v]"r" (currentThreadInfo->r2));\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r3));\
  asm("mov r4, %[v]" : : [v]"r" (currentThreadInfo->r4));\
  asm("mov r5, %[v]" : : [v]"r" (currentThreadInfo->r5));\
  asm("mov r6, %[v]" : : [v]"r" (currentThreadInfo->r6));\
  asm("mov r7, %[v]" : : [v]"r" (currentThreadInfo->r7));\
  asm("mov r8, %[v]" : : [v]"r" (currentThreadInfo->r8));\
  asm("mov r9, %[v]" : : [v]"r" (currentThreadInfo->r9));\
  asm("mov r10, %[v]" : : [v]"r" (currentThreadInfo->r10));\
  asm("mov r11, %[v]" : : [v]"r" (currentThreadInfo->r11));\
  asm("mov r12, %[v]" : : [v]"r" (currentThreadInfo->r12));\
  asm("LDM sp!, {pc}^")

//TODO extern "C" void arch_pageFaultHandler();
void pageFaultHandler(uint32 address, uint32 type)
{
  debug(PM, "[PageFaultHandler] Address: %x (%s) - currentThread: %x %d:%s, switch_to_userspace_: %d\n",
      address, type == 0x3 ? "Instruction Fetch" : "Data Access", currentThread, currentThread->getPID(), currentThread->getName(), currentThread->switch_to_userspace_);
  if (!currentThread->switch_to_userspace_)
    while(1);

  if(!address)
    debug(PM, "[PageFaultHandler] Maybe you're dereferencing a null-pointer!\n");

  ArchThreads::printThreadRegisters(currentThread,0);
  ArchThreads::printThreadRegisters(currentThread,1);

  //save previous state on stack of currentThread
  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;

  ArchInterrupts::enableInterrupts();
  if (currentThread->loader_)
  {
    //lets hope this Exeption wasn't thrown during a TaskSwitch
    if (address > 1U*1024U*1024U && address < 2U*1024U*1024U*1024U)
    {
      currentThread->loader_->loadOnePageSafeButSlow(address); //load stuff
    }
    else
    {
      debug(PM, "[PageFaultHandler] Memory Access Violation: address: %x, loader_: %x\n", address, currentThread->loader_);
      Syscall::exit(9999);
    }
  }
  else
  {
    debug(PM, "[PageFaultHandler] Kernel Page Fault! Should never happen...\n");
    currentThread->kill();
  }
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = true;
  currentThreadInfo = currentThread->user_arch_thread_info_;
}

void arch_uart0_irq_handler()
{
  kprintfd("arch_uart0_irq_handler\n");
  while(1);
}

void arch_uart1_irq_handler()
{
  kprintfd("arch_uart1_irq_handler\n");
  while(1);
}

void arch_keyboard_irq_handler()
{
  extern struct KMI* kmi;
  while (kmi->stat & 0x10)
  {
    KeyboardManager::instance()->serviceIRQ();
  }
}

void arch_mouse_irq_handler()
{
  kprintfd("arch_mouse_irq_handler\n");
  while(1);
}

void arch_timer0_irq_handler()
{
  kprintfd("timer\n");
  static uint32 heart_beat_value = 0;
  uint32 *t0mmio = (uint32*)0x83000000;
  if ((t0mmio[REG_INTSTAT] & 0x1) != 0)
  {
    assert(!ArchInterrupts::testIFSet());
    t0mmio[REG_INTCLR] = 1;     /* according to the docs u can write any value */

    const char* clock = "/-\\|";
    ((FrameBufferConsole*)main_console)->consoleSetCharacter(0,0,clock[heart_beat_value],0);
    heart_beat_value = (heart_beat_value + 1) % 4;

    Scheduler::instance()->incTicks();
    Scheduler::instance()->schedule();
  }
}

void arch_swi_irq_handler()
{
  assert(!ArchInterrupts::testIFSet());
  uint32 swi = *((uint32*)((uint32)currentThreadInfo->pc - 4)) & 0xffff;

  if (swi == 0xffff) // yield
  {
    Scheduler::instance()->schedule();
  }
  else
  {
    assert(false);
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThreadInfo->r0 = Syscall::syscallException(swi, currentThreadInfo->r0, currentThreadInfo->r1,
                                                           currentThreadInfo->r2, currentThreadInfo->r3,
                                                           currentThreadInfo->r4);
    ArchInterrupts::disableInterrupts();
    currentThread->switch_to_userspace_ = true;
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  }
}

#define IRQ(X) picmmio[PIC_IRQ_STATUS] & (1 << X)
extern "C" void switchTTBR0(uint32);

extern "C" void exceptionHandler(uint32 type) {
  kprintfd("exception type = %x\n", type);

  if (type == ARM4_XRQ_IRQ) {
    uint32* picmmio = (uint32*)0x84000000;
    if (IRQ(0))
      arch_swi_irq_handler();
    if (IRQ(1))
      arch_uart0_irq_handler();
    if (IRQ(2))
      arch_uart1_irq_handler();
    if (IRQ(3))
      arch_keyboard_irq_handler();
    if (IRQ(4))
      arch_mouse_irq_handler();
    if (IRQ(5))
      arch_timer0_irq_handler();
    // 6-10 and 22-28 not implemented
  }
  else if (type == ARM4_XRQ_SWINT) {
    arch_swi_irq_handler();
  }
  else if (type == ARM4_XRQ_ABRTP)
  {
    pageFaultHandler(currentThreadInfo->lr, type);
  }
  else if (type == ARM4_XRQ_ABRTD)
  {
    pageFaultHandler(currentThreadInfo->lr, type);
  }
  else {
    kprintfd("\nCPU Fault type = %x\n",type);
    ArchThreads::printThreadRegisters(currentThread,0);
    ArchThreads::printThreadRegisters(currentThread,1);
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    currentThread->kill();
    for(;;);
  }
  ArchThreads::printThreadRegisters(currentThread,0);
  ArchThreads::printThreadRegisters(currentThread,1);
  switchTTBR0(currentThreadInfo->ttbr0);
}
