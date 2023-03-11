#include "ArchThreads.h"

#include "Scheduler.h"
#include "SpinLock.h"
#include "Thread.h"
#include "kprintf.h"
#include "offsets.h"
#include "paging-definitions.h"

#include "ArchMemory.h"

SpinLock global_atomic_add_lock("global_atomic_add_lock");

extern PageDirEntry kernel_page_directory[];

void ArchThreads::initialise()
{
  new (&global_atomic_add_lock) SpinLock("global_atomic_add_lock");
  currentThreadRegisters = (ArchThreadRegisters*) new uint8[sizeof(ArchThreadRegisters)];
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)kernel_page_directory));
  currentThreadRegisters->ttbr0 = pageDirectory;
}

void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  assert(arch_memory.page_dir_page_ != 0);
  assert((arch_memory.page_dir_page_ & 0x3) == 0); // has to be aligned to 4 pages
  thread->kernel_registers_->ttbr0 = LOAD_BASE + arch_memory.page_dir_page_ * PAGE_SIZE;
  if (thread->user_registers_)
    thread->user_registers_->ttbr0 = LOAD_BASE + arch_memory.page_dir_page_ * PAGE_SIZE;
}

void ArchThreads::switchToAddressSpace(Thread* thread)
{
    ArchMemory::loadPagingStructureRoot(thread->kernel_registers_->ttbr0);
}

void ArchThreads::switchToAddressSpace(ArchMemory& arch_memory)
{
    size_t ttbr0_value = (LOAD_BASE + arch_memory.page_dir_page_ * PAGE_SIZE);
    ArchMemory::loadPagingStructureRoot(ttbr0_value);
}

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createKernelRegisters(void* start_function, void* stack)
{
  assert(!((pointer)start_function & 0x3));

  auto regs = eastl::make_unique<ArchThreadRegisters>();

  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)kernel_page_directory));
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);

  regs->pc = (pointer)start_function;
  regs->lr = (pointer)start_function;
  regs->cpsr = 0x6000001F;
  regs->sp = (pointer)stack & ~0xF;
  regs->r[11] = (pointer)stack & ~0xF; // r11 is the fp
  regs->ttbr0 = pageDirectory;

  return regs;
}

void ArchThreads::changeInstructionPointer(ArchThreadRegisters& info, void* function)
{
  info.pc = (pointer)function;
  info.lr = (pointer)function;
}

void* ArchThreads::getInstructionPointer(ArchThreadRegisters& info)
{
    return (void*)info.pc;
}

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createUserRegisters(void* start_function, void* user_stack, void* kernel_stack)
{
  assert(!((pointer)start_function & 0x3));

  auto regs = eastl::make_unique<ArchThreadRegisters>();

  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)kernel_page_directory));
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);

  regs->pc = (pointer)start_function;
  regs->lr = (pointer)start_function;
  regs->cpsr = 0x60000010;
  regs->sp = (pointer)user_stack & ~0xF;
  regs->r[11] = (pointer)user_stack & ~0xF; // r11 is the fp
  regs->sp0 = (pointer)kernel_stack & ~0xF;
  regs->ttbr0 = pageDirectory;

  return regs;
}

void ArchThreads::yield()
{
  asm("swi #0xffff");
}

extern "C" uint32 arch_TestAndSet(uint32, uint32, uint32 new_value, uint32 *lock);

void ArchThreads::printThreadRegisters(Thread *thread, bool verbose)
{
  printThreadRegisters(thread,0,verbose);
  printThreadRegisters(thread,1,verbose);
}

void ArchThreads::printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose)
{
  ArchThreadRegisters *info = userspace_registers ? thread->user_registers_.get() : thread->kernel_registers_.get();
  if (!info)
  {
    kprintfd("%sThread: %18p, has no %s registers. %s\n",userspace_registers?"  User":"Kernel",thread,userspace_registers?"User":"Kernel",userspace_registers?"":"This should never(!) occur. How did you do that?");
  }
  else if (verbose)
  {
    kprintfd("\t\t%sThread: %10p, info: %10p\n"\
             "\t\t\tttbr0: %10x  pc: %10x  sp: %10x  lr: %10x  cpsr: %10x\n"\
             "\t\t\tr0:%10x r1:%10x r2:%10x r3:%10x r4:%10x r5:%10x r6:%10x r7:%10x r8:%10x r9:%10x r10:%10x r11:%10x r12:%10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->ttbr0,info->pc,info->sp,info->lr,info->cpsr,info->r[0],info->r[1],info->r[2],info->r[3],info->r[4],info->r[5],info->r[6],info->r[7],info->r[8],info->r[9],info->r[10],info->r[11],info->r[12]);

  }
  else
  {
    kprintfd("%sThread: %10p, info: %10p -- ttbr0: %10x  pc: %10x  sp: %10x  lr: %10x  cpsr: %10x -- r0:%10x r1:%10x r2:%10x r3:%10x r4:%10x r5:%10x r6:%10x r7:%10x r8:%10x r9:%10x r10:%10x r11:%10x r12:%10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->ttbr0,info->pc,info->sp,info->lr,info->cpsr,info->r[0],info->r[1],info->r[2],info->r[3],info->r[4],info->r[5],info->r[6],info->r[7],info->r[8],info->r[9],info->r[10],info->r[11],info->r[12]);
  }
}


extern "C" void threadStartHack();

void ArchThreads::debugCheckNewThread(Thread* thread)
{
  assert(currentThread);
  ArchThreads::printThreadRegisters(currentThread,false);
  ArchThreads::printThreadRegisters(thread,false);
  assert(thread->kernel_registers_ != 0 && thread->kernel_registers_ != currentThread->kernel_registers_ && "all threads need to have their own register sets");
  assert(thread->kernel_registers_->sp0 == 0 && "kernel register set needs no backup of kernel esp");
  assert(thread->kernel_registers_->sp == thread->kernel_registers_->r[11] && "new kernel stack must be empty");
  assert(thread->kernel_registers_->sp != currentThread->kernel_registers_->sp && thread->kernel_registers_->r[11] != currentThread->kernel_registers_->r[11] && "all threads need their own stack");
  assert(thread->kernel_registers_->ttbr0 < 0x80000000 - BOARD_LOAD_BASE && "ttbr0 needs to contain the physical page dir address");
  if (thread->user_registers_ == 0)
    return;
  assert(thread->kernel_registers_->pc == 0 && "user threads should not start execution in kernel mode");
  assert(thread->switch_to_userspace_ == 1 && "new user threads must start in userspace");
  assert(thread->kernel_registers_->sp == thread->user_registers_->sp0 && "esp0 should point to kernel stack");
  assert(thread->kernel_registers_->ttbr0 == thread->user_registers_->ttbr0 && "user and kernel part of a thread need to have the same page dir");
  assert(thread->user_registers_->pc != 0 && "user eip needs to be valid... execution will start there");
  if (currentThread->user_registers_ == 0)
    return;
  assert(currentThread->user_registers_->sp0 != thread->user_registers_->sp0 && "no 2 threads may have the same esp0 value");
}

[[noreturn]] void ArchThreads::startThreads([[maybe_unused]]Thread* init_thread)
{
    ArchInterrupts::enableInterrupts();
    yield();
    assert(false);
}
