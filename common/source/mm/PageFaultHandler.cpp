#include "PageFaultHandler.h"
#include "kprintf.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "offsets.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Syscall.h"
#include "ArchThreads.h"
#include "ArchMulticore.h"
#include "ArchCommon.h"
#include "assert.h"

const size_t PageFaultHandler::null_reference_check_border_ = PAGE_SIZE;
size_t PageFaultHandler::pf_address = 0xDEADBEEF;
size_t PageFaultHandler::pf_address_counter = 0;

inline bool PageFaultHandler::checkPageFaultIsValid(size_t address, bool user,
                                                    bool present, bool switch_to_us)
{
  assert((user == switch_to_us) && "Thread is in user mode even though is should not be.");
  assert(!(address < USER_BREAK && currentThread->loader_ == nullptr) && "Thread accesses the user space, but has no loader.");
  assert(!(user && currentThread->user_registers_ == nullptr) && "Thread is in user mode, but has no valid registers.");

  if(address < null_reference_check_border_)
  {
    debug(PAGEFAULT, "Maybe you are dereferencing a null-pointer.\n");
  }
  else if(!user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing an invalid kernel address.\n");
  }
  else if(user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing a kernel address in user-mode.\n");
  }
  else if(present)
  {
    debug(PAGEFAULT, "You got a pagefault even though the address is mapped.\n");
  }
  else
  {
    // everything seems to be okay
    return true;
  }
  return false;
}

inline void PageFaultHandler::handlePageFault(size_t address, bool user,
                                          bool present, bool writing,
                                          bool fetch, bool switch_to_us)
{
  if (PAGEFAULT & OUTPUT_ENABLED)
    kprintfd("\n");
  debug(PAGEFAULT, "CPU %zu, Address: %18zx - Thread %zu: %s (%p)\n",
        ArchMulticore::getCpuID(), address, currentThread->getTID(), currentThread->getName(), currentThread);
  debug(PAGEFAULT, "Flags: %spresent, %s-mode, %s, %s-fetch, switch to userspace: %1d\n",
        present ? "    " : "not ",
        user ? "  user" : "kernel",
        writing ? "writing" : "reading",
        fetch ? "instruction" : "    operand",
        switch_to_us);

  ArchThreads::printThreadRegisters(currentThread, false);

  if (checkPageFaultIsValid(address, user, present, switch_to_us))
  {
    currentThread->loader_->loadPage(address);
  }
  else
  {
    // the page-fault seems to be faulty, print out the thread stack traces
    ArchThreads::printThreadRegisters(currentThread, true);
    currentThread->printBacktrace(true);
    if (currentThread->loader_)
    {
      Syscall::exit(9999);
    }
    else
    {
      currentThread->kill();
    }
  }
  debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
}

void PageFaultHandler::enterPageFault(size_t address, size_t ip, bool user,
                                      bool present, bool writing,
                                      bool fetch)
{
  debug(PAGEFAULT, "CPU %zu, Pagefault at %zx, ip %zx, present: %u, writing: %u, user: %u, instr fetch: %u\n", ArchMulticore::getCpuID(), address, ip, present, writing, user, fetch);
  assert(currentThread && "You have a pagefault, but no current thread");
  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;

  currentThread->switch_to_userspace_ = 0;
  currentThreadRegisters = currentThread->kernel_registers_;
  ArchInterrupts::enableInterrupts();

  handlePageFault(address, user, present, writing, fetch, saved_switch_to_userspace);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
  {
    currentThreadRegisters = currentThread->user_registers_;
  }
}


void PageFaultHandler::countPageFault(size_t address)
{
  if ((address ^ (size_t)currentThread) == pf_address)
  {
    pf_address_counter++;
  }
  else
  {
    pf_address = address ^ (size_t)currentThread;
    pf_address_counter = 0;
  }
  if (pf_address_counter >= 10)
  {
    kprintfd("same pagefault from the same thread for 10 times in a row. most likely you have an error in your code\n");
    ArchCommon::halt();
  }
}
