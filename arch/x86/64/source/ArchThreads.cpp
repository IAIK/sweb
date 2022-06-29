#include "ArchThreads.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "assert.h"
#include "Thread.h"
#include "kstring.h"
#include "ArchMulticore.h"
#include "Scheduler.h"

void ArchThreads::initialise()
{
  currentThreadRegisters = new ArchThreadRegisters{};

  /** Enable SSE for floating point instructions in long mode **/
  asm volatile ("movq %%cr0, %%rax\n"
          "and $0xFFFB, %%ax\n"
          "or $0x2, %%ax\n"
          "movq %%rax, %%cr0\n"
          "movq %%cr4, %%rax\n"
          "orq $0x200, %%rax\n"
          "movq %%rax, %%cr4\n" : : : "rax");
}
void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  assert(arch_memory.getPagingStructureRootPhys());
  thread->kernel_registers_->cr3 = arch_memory.getValueForCR3();
  if (thread->user_registers_)
    thread->user_registers_->cr3 = arch_memory.getValueForCR3();

  if(thread == currentThread)
  {
    switchToAddressSpace(arch_memory);
  }
}

void ArchThreads::switchToAddressSpace(Thread* thread)
{
  ArchMemory::loadPagingStructureRoot(thread->kernel_registers_->cr3);
}

void ArchThreads::switchToAddressSpace(ArchMemory& arch_memory)
{
  ArchMemory::loadPagingStructureRoot(arch_memory.getValueForCR3());
}


WithAddressSpace::WithAddressSpace(Thread* thread) :
    prev_addr_space_(0)
{
    if (thread)
    {
        prev_addr_space_ = thread->kernel_registers_->cr3;
        ArchThreads::switchToAddressSpace(thread);
    }
}

WithAddressSpace::WithAddressSpace(ArchMemory& arch_memory)
{
    prev_addr_space_ = arch_memory.getValueForCR3();
    ArchThreads::switchToAddressSpace(arch_memory);
}

WithAddressSpace::~WithAddressSpace()
{
    if (prev_addr_space_)
    {
        ArchMemory::loadPagingStructureRoot(prev_addr_space_);
    }
}

void ArchThreads::createBaseThreadRegisters(ArchThreadRegisters *&info, void* start_function, void* stack)
{
  info = new ArchThreadRegisters{};

  info->rflags  = 0x200; // interrupt enable flag set
  info->cr3     = kernel_arch_mem.getValueForCR3();
  info->rsp     = (size_t)stack;
  info->rbp     = (size_t)stack;
  info->rip     = (size_t)start_function;

  /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
}

void ArchThreads::createKernelRegisters(ArchThreadRegisters *&info, void* start_function, void* kernel_stack)
{
  createBaseThreadRegisters(info, start_function, kernel_stack);

  info->cs      = KERNEL_CS;
  info->ds      = KERNEL_DS;
  info->es      = KERNEL_DS;
  info->ss      = KERNEL_SS;
  info->rsp0    = (size_t)kernel_stack;
  assert(info->cr3);
}

void ArchThreads::createUserRegisters(ArchThreadRegisters *&info, void* start_function, void* user_stack, void* kernel_stack)
{
  createBaseThreadRegisters(info, start_function, user_stack);

  info->cs      = USER_CS;
  info->ds      = USER_DS;
  info->es      = USER_DS;
  info->ss      = USER_SS;
  info->rsp0    = (size_t)kernel_stack;
  assert(info->cr3);
}

void ArchThreads::changeInstructionPointer(ArchThreadRegisters *info, void* function)
{
  info->rip = (size_t)function;
}

void* ArchThreads::getInstructionPointer(ArchThreadRegisters *info)
{
        return (void*)info->rip;
}

void ArchThreads::yield()
{
  __asm__ __volatile__("int $65");
}

uint64 ArchThreads::atomic_add(uint64 &value, int64 increment)
{
  int64 ret=increment;
  __asm__ __volatile__(
  "lock; xaddq %0, %1;"
  :"=a" (ret), "=m" (value)
  :"a" (ret)
  :);
  return ret;
}

uint32 ArchThreads::atomic_add(uint32 &value, int32 increment)
{
    return __sync_fetch_and_add(&value,increment);
}

int32 ArchThreads::atomic_add(int32 &value, int32 increment)
{
    return __sync_fetch_and_add(&value,increment);
}

int64 ArchThreads::atomic_add(int64 &value, int64 increment)
{
  return (int64) ArchThreads::atomic_add((uint64 &) value, increment);
}

size_t ArchThreads::atomic_add(size_t &value, ssize_t increment)
{
  return (size_t) ArchThreads::atomic_add(*(uint64_t*)&value, (int64_t)increment);
}

void ArchThreads::atomic_set(uint32& target, uint32 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(int32& target, int32 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(uint64& target, uint64 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(size_t& target, size_t value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(int64& target, int64 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::printThreadRegisters(Thread *thread, bool verbose)
{
  printThreadRegisters(thread,0,verbose);
  printThreadRegisters(thread,1,verbose);
}

void ArchThreads::printThreadRegisters(Thread *thread, size_t userspace_registers, bool verbose)
{
  ArchThreadRegisters *info = userspace_registers?thread->user_registers_:thread->kernel_registers_;
  if (!info)
  {
    kprintfd("%sThread: %18p, has no %s registers. %s\n",userspace_registers?"  User":"Kernel",thread,userspace_registers?"User":"Kernel",userspace_registers?"":"This should never(!) occur. How did you do that?");
  }
  else if (verbose)
  {
    kprintfd("\t\t%sThread: %18p, info: %18p\n"\
             "\t\t\t rax: %18llx  rbx: %18llx  rcx: %18llx  rdx: %18llx\n"\
             "\t\t\t rsp: %18llx  rbp: %18llx  rsp0 %18llx  rip: %18llx\n"\
             "\t\t\trflg: %18llx  cr3: %18llx\n",
             userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);
  }
  else
  {
    kprintfd("%sThread: %18p, info: %18p -- rax: %18llx  rbx: %18llx  rcx: %18llx  rdx: %18llx -- rsp: %18llx  rbp: %18llx  rsp0 %18llx -- rip: %18llx  rflg: %18llx  cr3: %llx\n",
             userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);
  }
}

extern "C" void threadStartHack();

void ArchThreads::debugCheckNewThread(Thread* thread)
{
  assert(currentThread);
  ArchThreads::printThreadRegisters(currentThread, false);
  ArchThreads::printThreadRegisters(thread, false);
  assert(thread->kernel_registers_ != 0 && thread->kernel_registers_ != currentThread->kernel_registers_ && "all threads need to have their own register sets");
  assert(thread->kernel_registers_->rsp == thread->kernel_registers_->rbp && "new kernel stack must be empty");
  assert(thread->kernel_registers_->rsp != currentThread->kernel_registers_->rsp && thread->kernel_registers_->rbp != currentThread->kernel_registers_->rbp && "all threads need their own stack");
  assert(thread->kernel_registers_->cr3 < 0x80000000 && "cr3 contains the physical page dir address");
  if (thread->user_registers_ == 0)
    return;
  assert(thread->kernel_registers_->rip == 0 && "user threads should not start execution in kernel mode");
  assert(thread->switch_to_userspace_ == 1 && "new user threads must start in userspace");
  assert(thread->kernel_registers_->rsp == thread->user_registers_->rsp0 && "esp0 should point to kernel stack");
  assert(thread->kernel_registers_->cr3 == thread->user_registers_->cr3 && "user and kernel part of a thread need to have the same page dir");
  assert(thread->user_registers_->rip != 0 && "user eip needs to be valid... execution will start there");
  if (currentThread->user_registers_ == 0)
    return;
  assert(currentThread->user_registers_->rsp0 != thread->user_registers_->rsp0 && "no 2 threads may have the same esp0 value");
}



[[noreturn]] void ArchThreads::startThreads(Thread* init_thread)
{
    contextSwitch(init_thread, init_thread->kernel_registers_);
}
