#pragma once

#include "types.h"
#include "Scheduler.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadRegisters
{
  uint64  rip;       //   0
  uint64  cs;        //   8
  uint64  rflags;    //  16
  uint64  rax;       //  24
  uint64  rcx;       //  32
  uint64  rdx;       //  40
  uint64  rbx;       //  48
  uint64  rsp;       //  56
  uint64  rbp;       //  64
  uint64  rsi;       //  72
  uint64  rdi;       //  80
  uint64  r8;        //  88
  uint64  r9;        //  96
  uint64  r10;       // 104
  uint64  r11;       // 112
  uint64  r12;       // 120
  uint64  r13;       // 128
  uint64  r14;       // 136
  uint64  r15;       // 144
  uint64  ds;        // 152
  uint64  es;        // 160
  uint64  fs;        // 168
  uint64  gs;        // 176
  uint64  ss;        // 184
  uint64  rsp0;      // 192
  uint64  cr3;       // 200
  uint32  fpu[28];   // 208
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
  static void createKernelRegisters(ArchThreadRegisters *&info, void* start_function, void* kernel_stack);

/**
 * creates the ArchThreadRegisters for a user thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param user_stack pointer to the userstack
 * @param kernel_stack pointer to the kernel stack
 */
  static void createUserRegisters(ArchThreadRegisters *&info, void* start_function, void* user_stack, void* kernel_stack);

/**
 * changes an existing ArchThreadRegisters so that execution will start / continue
 * at the function specified
 * it does not change anything else, and if the thread info / thread was currently
 * executing something else this will lead to a lot of problems
 * USE WITH CARE, or better, don't use at all if you're a student
 * @param info the ArchThreadRegisters that we are going to mangle
 * @param start_function instruction pointer for the next instruction that gets executed
 */
  static void changeInstructionPointer(ArchThreadRegisters *info, void* function);

  static void* getInstructionPointer(ArchThreadRegisters *info);

  static void setInterruptEnableFlag(ArchThreadRegisters *info, bool interrupts_enabled);

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
  static void createBaseThreadRegisters(ArchThreadRegisters *&info, void* start_function, void* stack);
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
