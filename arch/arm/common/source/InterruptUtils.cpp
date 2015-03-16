#include "InterruptUtils.h"

#include "ArchBoardSpecific.h"
#include "BDManager.h"
#include "KeyboardManager.h"
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

#include "panic.h"

#include "Thread.h"
#include "ArchInterrupts.h"
#include "backtrace.h"

#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"

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
      address, type == 0x3 ? "Instruction Fetch" : "Data Access", currentThread, currentThread->getTID(), currentThread->getName(), currentThread->switch_to_userspace_);
  if (!currentThread->switch_to_userspace_)
  {
    currentThread->printBacktrace(true);
    ArchThreads::printThreadRegisters(currentThread,false);
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

  ArchThreads::printThreadRegisters(currentThread,false);

  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;
  currentThread->switch_to_userspace_ = 0;
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
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
    currentThreadInfo = currentThread->user_arch_thread_info_;
}

void timer_irq_handler()
{
  static uint32 heart_beat_value = 0;
  const char* clock = "/-\\|";
  ((FrameBufferConsole*)main_console)->consoleSetCharacter(0,0,clock[heart_beat_value],Console::GREEN);
  heart_beat_value = (heart_beat_value + 1) % 4;

  Scheduler::instance()->incTicks();
  Scheduler::instance()->schedule();
}

void arch_uart1_irq_handler()
{
  kprintfd("arch_uart1_irq_handler\n");
  while(1);
}

void arch_mouse_irq_handler()
{
  kprintfd("arch_mouse_irq_handler\n");
  while(1);
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
    currentThread->switch_to_userspace_ = 0;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThread->user_arch_thread_info_->r[0] = Syscall::syscallException(currentThread->user_arch_thread_info_->r[0],
                                                                          currentThread->user_arch_thread_info_->r[1],
                                                                          currentThread->user_arch_thread_info_->r[2],
                                                                          currentThread->user_arch_thread_info_->r[3],
                                                                          currentThread->user_arch_thread_info_->r[4],
                                                                          currentThread->user_arch_thread_info_->r[5]);
    ArchInterrupts::disableInterrupts();
    currentThread->switch_to_userspace_ = 1;
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  }
  else
  {
    kprintfd("Invalid SWI: %x\n",swi);
    assert(false);
  }
}

extern "C" void switchTTBR0(uint32);

extern "C" void exceptionHandler(uint32 type)
{
  assert(!currentThread || currentThread->stack_[0] == STACK_CANARY);
  debug(A_INTERRUPTS, "InterruptUtils::exceptionHandler: type = %x\n", type);
  assert((currentThreadInfo->cpsr & (0xE0)) == 0);
  if (!currentThread)
  {
    Scheduler::instance()->schedule();
  }
  else if (type == ARM4_XRQ_IRQ) {
    ArchBoardSpecific::irq_handler();
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
    ArchThreads::printThreadRegisters(currentThread,false);
    currentThread->switch_to_userspace_ = 0;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThread->kill();
    for(;;);
  }
//  ArchThreads::printThreadRegisters(currentThread,false);
  assert((currentThreadInfo->ttbr0 & 0x3FFF) == 0 && (currentThreadInfo->ttbr0 & ~0x3FFF) != 0);
  assert((currentThreadInfo->cpsr & 0xE0) == 0);
  assert(currentThread->switch_to_userspace_ == 0 || (currentThreadInfo->cpsr & 0xF) == 0);
  assert(!currentThread || currentThread->stack_[0] == STACK_CANARY);
  switchTTBR0(currentThreadInfo->ttbr0);
}
