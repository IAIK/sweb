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
#include "EASTL/unique_ptr.h"

void ArchThreads::initialise()
{
    // Required for interrupts
    static ArchThreadRegisters boot_thread_registers{};
    currentThreadRegisters = &boot_thread_registers;

    /** Enable SSE for floating point instructions in long mode **/
    asm volatile("movq %%cr0, %%rax\n"
                 "and $0xFFFB, %%ax\n"
                 "or $0x2, %%ax\n"
                 "movq %%rax, %%cr0\n"
                 "movq %%cr4, %%rax\n"
                 "orq $0x200, %%rax\n"
                 "movq %%rax, %%cr4\n"
                 :
                 :
                 : "rax");
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

eastl::unique_ptr<ArchThreadRegisters> ArchThreads::createBaseThreadRegisters(
    void* start_function, void* stack)
{
    auto regs = eastl::make_unique<ArchThreadRegisters>();

    setInterruptEnableFlag(*regs, true);
    regs->cr3 = ArchMemory::kernelArchMemory().getValueForCR3();
    regs->rsp = (size_t)stack;
    regs->rbp = (size_t)stack;
    regs->rip = (size_t)start_function;

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
    kregs->rsp0 = (size_t)kernel_stack;
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
    uregs->rsp0 = (size_t)kernel_stack;
    assert(uregs->cr3);
    return uregs;
}

void ArchThreads::changeInstructionPointer(ArchThreadRegisters& info, void* function)
{
  info.rip = (size_t)function;
}

void* ArchThreads::getInstructionPointer(ArchThreadRegisters& info)
{
    return (void*)info.rip;
}

void ArchThreads::setInterruptEnableFlag(ArchThreadRegisters& info, bool interrupts_enabled)
{
    if (interrupts_enabled)
        info.rflags |= 0x200;
    else
        info.rflags &= ~0x200;
}

bool ArchThreads::getInterruptEnableFlag(ArchThreadRegisters& info)
{
    return info.rflags & 0x200;
}

void ArchThreads::yield()
{
  __asm__ __volatile__("int $65");
}

void ArchThreads::printThreadRegisters(Thread *thread, bool verbose)
{
  printThreadRegisters(thread,0,verbose);
  printThreadRegisters(thread,1,verbose);
}

void ArchThreads::printThreadRegisters(Thread *thread, size_t userspace_registers, bool verbose)
{
  ArchThreadRegisters *info = userspace_registers ? thread->user_registers_.get() : thread->kernel_registers_.get();
  if (!info)
  {
    kprintfd("%sThread: %18p, has no %s registers. %s\n",userspace_registers?"  User":"Kernel",thread,userspace_registers?"User":"Kernel",userspace_registers?"":"This should never(!) occur. How did you do that?");
  }
  else if (verbose)
  {
    kprintfd("\t\t%sThread: %18p, info: %18p\n"\
             "\t\t\t rax: %18lx  rbx: %18lx  rcx: %18lx  rdx: %18lx\n"\
             "\t\t\t rsp: %18lx  rbp: %18lx  rsp0 %18lx  rip: %18lx\n"\
             "\t\t\trflg: %18lx  cr3: %18lx\n",
             userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);
  }
  else
  {
    kprintfd("%sThread: %18p, info: %18p -- rax: %18lx  rbx: %18lx  rcx: %18lx  rdx: %18lx -- rsp: %18lx  rbp: %18lx  rsp0 %18lx -- rip: %18lx  rflg: %18lx  cr3: %lx\n",
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
    contextSwitch(init_thread, init_thread->kernel_registers_.get());
}
