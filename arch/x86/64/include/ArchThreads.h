#pragma once

#include "Scheduler.h"
#include "types.h"
#include "EASTL/unique_ptr.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadRegisters
{
  uint64  rip;
  uint64  cs;
  uint64  rflags;
  uint64  rax;
  uint64  rcx;
  uint64  rdx;
  uint64  rbx;
  uint64  rsp;
  uint64  rbp;
  uint64  rsi;
  uint64  rdi;
  uint64  r8;
  uint64  r9;
  uint64  r10;
  uint64  r11;
  uint64  r12;
  uint64  r13;
  uint64  r14;
  uint64  r15;
  uint64  ds;
  uint64  es;
  uint64  fs;
  uint64  gs;
  uint64  ss;
  uint64  rsp0;
  uint64  cr3;
  uint64  fsbase;
  uint32  fpu[28];

    void setKernelStack(size_t k_stack)
    {
        rsp0 = k_stack;
    }

    void setStack(size_t stack)
    {
        rsp = stack;
        rbp = stack;
    }
};

class Thread;
class ArchMemory;

/**
 * Collection of architecture dependant code concerning Task Switching
 *
 */
class ArchThreads
{
public:

  [[noreturn]] static void startThreads(Thread* init_thread);

/**
 * allocates space for the currentThreadRegisters
 */
  static void initialise();

/**
 * creates the ArchThreadRegisters for a kernel thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param stack stackpointer
 */
  static eastl::unique_ptr<ArchThreadRegisters> createKernelRegisters(void* start_function,
                                                                      void* kernel_stack);

  /**
   * creates the ArchThreadRegisters for a user thread
   * @param info where the ArchThreadRegisters is saved
   * @param start_function instruction pointer is set so start function
   * @param user_stack pointer to the userstack
   * @param kernel_stack pointer to the kernel stack
   */
  static eastl::unique_ptr<ArchThreadRegisters> createUserRegisters(void* start_function,
                                                                    void* user_stack,
                                                                    void* kernel_stack);

  /**
   * changes an existing ArchThreadRegisters so that execution will start / continue
   * at the function specified
   * it does not change anything else, and if the thread info / thread was currently
   * executing something else this will lead to a lot of problems
   * USE WITH CARE, or better, don't use at all if you're a student
   * @param info the ArchThreadRegisters that we are going to mangle
   * @param start_function instruction pointer for the next instruction that gets executed
   */
  static void changeInstructionPointer(ArchThreadRegisters& info, void* function);

  static void* getInstructionPointer(ArchThreadRegisters& info);

  static void setInterruptEnableFlag(ArchThreadRegisters& info, bool interrupts_enabled);
  static bool getInterruptEnableFlag(ArchThreadRegisters& info);

/**
 * on x86: invokes int65, whose handler facilitates a task switch
 */
  static void yield();

/**
 * sets a threads page map level 4
 *
 * @param thread Pointer to Thread Object
 * @param arch_memory the arch memory object for the address space
 */
  static void setAddressSpace(Thread *thread, ArchMemory& arch_memory);

  static void switchToAddressSpace(Thread* thread);
  static void switchToAddressSpace(ArchMemory& arch_memory);

/**
 * uninterruptable locked operation
 * exchanges value in variable lock with new_value and returns the old_value
 *
 * @param &lock Reference to variable being tested
 * @param new_value to set variable lock to
 * @returns old_value of variable lock
 */
  template<typename T>
  static T testSetLock(volatile T &lock, T new_value)
  {
      return __sync_lock_test_and_set(&lock, new_value);
  }

 /**
  * Counterpart to testSetLock()
  * Writes 0 to the lock variable and provides a memory release barrier
  * (ensures all previous memory stores are visible)
  */
  template<typename T>
  static void syncLockRelease(volatile T &lock)
  {
      __sync_lock_release(&lock);
  }

/**
 * atomically increments or decrements target by increment
 *
 * @param &target Reference to target
 * @param increment can be positive or negative
 * @returns old value of target
 */
  template <typename T>
  static T atomic_add(T& target, T increment)
  {
      return __atomic_fetch_add(&target, increment, __ATOMIC_SEQ_CST);
  }

  /**
   * Atomically set a target to another value.
   *
   * @param target The target which shall be set
   * @param value The value which shall be set
   */
  template <typename T>
  static void atomic_set(T& target, T value)
  {
      __atomic_store_n(&target, value, __ATOMIC_SEQ_CST);
  }

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, size_t userspace_registers, bool verbose = true);
  static void printThreadRegisters(Thread *thread, bool verbose = true);

  /**
   * check thread state for sanity
   * @param thread
   */
  static void debugCheckNewThread(Thread* thread);

private:
  /**
   * creates the ArchThreadRegisters for a thread (common setup for kernel and user registers)
   * @param info where the ArchThreadRegisters is saved
   * @param start_function instruction pointer is set to start function
   * @param stack stackpointer
   */
  static eastl::unique_ptr<ArchThreadRegisters> createBaseThreadRegisters(void* start_function, void* stack);
};

class WithAddressSpace
{
public:
    WithAddressSpace(Thread* thread);
    WithAddressSpace(ArchMemory& arch_memory);
    ~WithAddressSpace();
private:
    size_t prev_addr_space_;
};
