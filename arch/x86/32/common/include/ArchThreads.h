#pragma once

#include "types.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadRegisters
{
  uint32  eip;
  uint32  cs;
  uint32  eflags;
  uint32  eax;
  uint32  ecx;
  uint32  edx;
  uint32  ebx;
  uint32  esp;
  uint32  ebp;
  uint32  esi;
  uint32  edi;
  uint32  ds;
  uint32  es;
  uint32  fs;
  uint32  gs;
  uint32  ss;
  uint32  esp0;
  uint32  cr3;
  uint32  fpu[27];
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
 * @param the ArchThreadRegisters that we are going to mangle
 * @param start_function instruction pointer for the next instruction that gets executed
 */
  static void changeInstructionPointer(ArchThreadRegisters *info, void* function);

  static void* getInstructionPointer(ArchThreadRegisters *info);

/**
 *
 * on x86: invokes int65, whose handler facilitates a task switch
 *
 */
  static void yield();

/**
 * sets a threads CR3 register to the given page dir / etc. defining its address space
 *
 * @param *thread Pointer to Thread Object
 * @param arch_memory a reference to the arch memory object to use
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
 * atomically increments or decrements value by increment
 *
 * @param &value Reference to value
 * @param increment can be positive or negative
 * @returns old value of value
 */
  static uint32 atomic_add(uint32 &value, int32 increment);
  static int32 atomic_add(int32 &value, int32 increment);
  static uint64 atomic_add(uint64 &value, int64 increment);
  static int64 atomic_add(int64 &value, int64 increment);

  /**
   * Atomically set a target to another value.
   *
   * @param target The target which shall be set
   * @param value The value which shall be set
   */
  static void atomic_set(uint32 &target, uint32 value);
  static void atomic_set(int32 &target, int32 value);
  static void atomic_set(uint64 &target, uint64 value);
  static void atomic_set(int64 &target, int64 value);

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose = true);
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
