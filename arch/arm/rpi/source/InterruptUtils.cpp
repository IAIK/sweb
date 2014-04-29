/**
 * @file InterruptUtils.cpp
 *
 */

#include "InterruptUtils.h"
#include "new.h"
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

uint32 last_address;
uint32 count;

//TODO extern "C" void arch_pageFaultHandler();
void pageFaultHandler(uint32 address, uint32 type)
{
  if (type != 0x3)
  {
    asm("mrc p15, 0, r4, c6, c0, 0\n\
         mov %[v], r4\n": [v]"=r" (address));
  }
  debug(PM, "[PageFaultHandler] Address: %x (%s) - currentThread: %x %d:%s, switch_to_userspace_: %d\n",
      address, type == 0x3 ? "Instruction Fetch" : "Data Access", currentThread, currentThread->getPID(), currentThread->getName(), currentThread->switch_to_userspace_);
  if (!currentThread->switch_to_userspace_)
  {
    currentThread->printBacktrace(true);
    ArchThreads::printThreadRegisters(currentThread,0);
    ArchThreads::printThreadRegisters(currentThread,1);
    while(1);
  }

  if(!address)
    debug(PM, "[PageFaultHandler] Maybe you're dereferencing a null-pointer!\n");

  if(currentThread->loader_->arch_memory_.checkAddressValid(address))
    debug(PM, "[PageFaultHandler] There is something wrong: The address is actually mapped!\n");

  if (address != last_address)
  {
    count = 0;
    last_address = address;
  }
  else
  {
    if (count++ == 5)
    {
      debug(PM, "[PageFaultHandler] 5 times the same pagefault? That should not happen -> kill Thread\n");
      currentThread->kill();
    }
  }

  ArchThreads::printThreadRegisters(currentThread,0);
  ArchThreads::printThreadRegisters(currentThread,1);

  //save previous state on stack of currentThread
  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;

  ArchInterrupts::enableInterrupts();
  if (currentThread->loader_)
  {
    //lets hope this Exeption wasn't thrown during a TaskSwitch
    if (address > 8U*1024U*1024U && address < 2U*1024U*1024U*1024U)
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
  KeyboardManager::instance()->serviceIRQ();
}

void arch_mouse_irq_handler()
{
  kprintfd("arch_mouse_irq_handler\n");
  while(1);
}

void arch_timer0_irq_handler()
{
  static uint32 heart_beat_value = 0;
  uint32* timer_raw = (uint32*)0x9000B410;
  if ((*timer_raw & 0x1) != 0)
  {
    assert(!ArchInterrupts::testIFSet());
    uint32* timer_clear = (uint32*)0x9000B40C;
    *timer_clear = 0x1;

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
  else if (swi == 0x0) // syscall
  {
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThread->user_arch_thread_info_->r0 = Syscall::syscallException(currentThread->user_arch_thread_info_->r4,
                                                                          currentThread->user_arch_thread_info_->r5,
                                                                          currentThread->user_arch_thread_info_->r6,
                                                                          currentThread->user_arch_thread_info_->r7,
                                                                          currentThread->user_arch_thread_info_->r8,
                                                                          currentThread->user_arch_thread_info_->r9);
    ArchInterrupts::disableInterrupts();
    currentThread->switch_to_userspace_ = true;
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  }
  else
  {
    kprintfd("Invalid SWI: %x\n",swi);
    assert(false);
  }
}

#define IRQ(X) ((*pic) & (1 << X))
extern "C" void switchTTBR0(uint32);

extern "C" void exceptionHandler(uint32 type)
{
  debug(A_INTERRUPTS, "InterruptUtils::exceptionHandler: type = %x\n", type);
  currentThreadInfo->cpsr &= ~(0xE0);
  if (type == ARM4_XRQ_IRQ) {
    uint32* pic = (uint32*)0x9000B200;
    if (IRQ(0))
      arch_timer0_irq_handler();
  }
  else if (type == ARM4_XRQ_SWINT) {
    arch_swi_irq_handler();
  }
  else if (type == ARM4_XRQ_ABRTP)
  {
    pageFaultHandler(currentThreadInfo->pc, type);
  }
  else if (type == ARM4_XRQ_ABRTD)
  {
    pageFaultHandler(currentThreadInfo->pc, type);
  }
  else {
    kprintfd("\nCPU Fault type = %x\n",type);
    ArchThreads::printThreadRegisters(currentThread,0);
    ArchThreads::printThreadRegisters(currentThread,1);
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThread->kill();
    for(;;);
  }
//  ArchThreads::printThreadRegisters(currentThread,0);
//  ArchThreads::printThreadRegisters(currentThread,1);
  assert((currentThreadInfo->ttbr0 & 0x3FFF) == 0 && (currentThreadInfo->ttbr0 & ~0x3FFF) != 0);
  assert((currentThreadInfo->cpsr & 0xE0) == 0);
  switchTTBR0(currentThreadInfo->ttbr0);
}
