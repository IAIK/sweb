#include "ArchThreads.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "Thread.h"
#include "kstring.h"
#include "SegmentUtils.h"



void ArchThreads::initialise()
{
  currentThreadRegisters = new ArchThreadRegisters{};
}

void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
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

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createBaseThreadRegisters(
    void* start_function, void* stack)
{
    auto regs = eastl::make_unique<ArchThreadRegisters>();

    setInterruptEnableFlag(*regs, true);
    regs->cr3 = kernel_arch_mem.getValueForCR3();
    regs->esp = (size_t)stack;
    regs->ebp = (size_t)stack;
    regs->eip = (size_t)start_function;

    /* fpu (=fninit) */
    regs->fpu[0] = 0xFFFF037F;
    regs->fpu[1] = 0xFFFF0000;
    regs->fpu[2] = 0xFFFFFFFF;
    regs->fpu[3] = 0x00000000;
    regs->fpu[4] = 0x00000000;
    regs->fpu[5] = 0x00000000;
    regs->fpu[6] = 0xFFFF0000;

    return regs;
}

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createKernelRegisters(
    void* start_function, void* kernel_stack)
{
    auto kregs = createBaseThreadRegisters(start_function, kernel_stack);

    kregs->cs = KERNEL_CS;
    kregs->ds = KERNEL_DS;
    kregs->es = KERNEL_DS;
    kregs->ss = KERNEL_SS;
    kregs->fs = KERNEL_FS;
    kregs->gs = KERNEL_GS;
    assert(kregs->cr3);

    return kregs;
}

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createUserRegisters(
    void* start_function, void* user_stack, void* kernel_stack)
{
    auto uregs = createBaseThreadRegisters(start_function, user_stack);

    uregs->cs = USER_CS;
    uregs->ds = USER_DS;
    uregs->es = USER_DS;
    uregs->ss = USER_SS;
    uregs->fs = USER_DS;
    uregs->gs = USER_DS;
    uregs->esp0 = (size_t)kernel_stack;
    assert(uregs->cr3);

    return uregs;
}

void ArchThreads::changeInstructionPointer(ArchThreadRegisters& info, void* function)
{
  info.eip = (size_t)function;
}

void* ArchThreads::getInstructionPointer(ArchThreadRegisters& info)
{
  return (void*)info.eip;
}

void ArchThreads::setInterruptEnableFlag(ArchThreadRegisters& info,
                                         bool interrupts_enabled)
{
    if (interrupts_enabled)
        info.eflags |= 0x200;
    else
        info.eflags &= ~0x200;
}

void ArchThreads::yield()
{
  __asm__ __volatile__("int $65");
}

uint32 ArchThreads::atomic_add(uint32 &value, int32 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

int32 ArchThreads::atomic_add(int32 &value, int32 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

uint64 ArchThreads::atomic_add(uint64 &value, int64 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

int64 ArchThreads::atomic_add(int64 &value, int64 increment)
{
  return __sync_fetch_and_add(&value,increment);
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

void ArchThreads::atomic_set(int64& target, int64 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}


void ArchThreads::printThreadRegisters(Thread *thread, bool verbose)
{
  printThreadRegisters(thread, 0, verbose);
  printThreadRegisters(thread, 1, verbose);
}

void ArchThreads::printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose)
{
  ArchThreadRegisters *info = (userspace_registers ? thread->user_registers_.get() : thread->kernel_registers_.get());
  if (!info)
  {
    kprintfd("%sThread: %18p, has no %s registers. %s\n",userspace_registers?"  User":"Kernel",thread,userspace_registers?"User":"Kernel",userspace_registers?"":"This should never(!) occur. How did you do that?");
  }
  else if (verbose)
  {
    kprintfd("\t\t%sThread: %10p, info: %10p\n"\
             "\t\t\t eax: %#10x  ebx: %#10x  ecx: %#10x  edx: %#10x\n"\
             "\t\t\t esi: %#10x  edi: %#10x  esp: %#10x  ebp: %#10x\n"\
             "\t\t\tesp0: %#10x  eip: %#10x eflg: %#10x  cr3: %#10x\n"\
             "\t\t\t  ds: %#10x   ss: %#10x   es: %#10x   fs: %#10x\n"\
             "\t\t\t  gs: %#10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->eax,info->ebx,info->ecx,info->edx,info->esi,info->edi,info->esp,info->ebp,info->esp0,info->eip,info->eflags,info->cr3,info->ds,info->ss,info->es,info->fs,info->gs);
  }
  else
  {
    kprintfd("\t%sThread %10p: info %10p eax %#10x ebp %#10x esp %#10x esp0 %#10x eip %#10x cr3 %#10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->eax,info->ebp,info->esp,info->esp0,info->eip,info->cr3);
  }
}

extern "C" void threadStartHack();

void ArchThreads::debugCheckNewThread(Thread* thread)
{
  assert(currentThread);
  ArchThreads::printThreadRegisters(currentThread,false);
  ArchThreads::printThreadRegisters(thread,false);
  assert(thread->kernel_registers_ != 0 && thread->kernel_registers_ != currentThread->kernel_registers_ && "all threads need to have their own register sets");
  assert(thread->kernel_registers_->esp0 == 0 && "kernel register set needs no backup of kernel esp");
  assert(thread->kernel_registers_->esp == thread->kernel_registers_->ebp && "new kernel stack must be empty");
  assert(thread->kernel_registers_->esp != currentThread->kernel_registers_->esp && thread->kernel_registers_->ebp != currentThread->kernel_registers_->ebp && "all threads need their own stack");
  assert(thread->kernel_registers_->cr3 < 0x80000000 && "cr3 contains the physical page dir address");
  if (thread->user_registers_ == 0)
    return;
  assert(thread->kernel_registers_->eip == 0 && "user threads should not start execution in kernel mode");
  assert(thread->switch_to_userspace_ == 1 && "new user threads must start in userspace");
  assert(thread->kernel_registers_->esp == thread->user_registers_->esp0 && "esp0 should point to kernel stack");
  assert(thread->kernel_registers_->cr3 == thread->user_registers_->cr3 && "user and kernel part of a thread need to have the same page dir");
  assert(thread->kernel_registers_->cr3 == thread->loader_->arch_memory_.getValueForCR3() && "thread and loader need to have the same page dir");
  assert(thread->user_registers_->eip != 0 && "user eip needs to be valid... execution will start there");
  if (currentThread->user_registers_ == 0)
    return;
  assert(currentThread->user_registers_->esp0 != thread->user_registers_->esp0 && "no 2 threads may have the same esp0 value");
}


[[noreturn]] void ArchThreads::startThreads(Thread* init_thread)
{
    contextSwitch(init_thread, init_thread->kernel_registers_.get());
}
