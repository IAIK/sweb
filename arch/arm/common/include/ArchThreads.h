/**
 * @file ArchThreads.h
 *
 */

#ifndef _ARCH_THREADS_H_
#define _ARCH_THREADS_H_

#include "types.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadInfo
{
  uint32 r[13];
  uint32 sp;
  uint32 lr;
  uint32 cpsr;
  uint32 spsr;
  uint32 pc;
  uint32 ttbr0;
  uint32 sp0;
};

class Thread;
class ArchMemory;
/**
 * this is where the thread info for task switching is stored
 *
 */
extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

/**
 * Collection of architecture dependant code concerning Task Switching
 *
 */
class ArchThreads
{
public:

/**
 * allocates space for the currentThreadInfo
 *
 */
  static void initialise();

/**
 * creates the ArchThreadInfo for a kernel thread
 * @param info where the ArchThreadInfo is saved
 * @param start_function instruction pointer is set so start function
 * @param stack stackpointer
 */
  static void createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack);

  /**
   * changes an existing ArchThreadInfo so that execution will start / continue
   * at the function specified
   * it does not change anything else, and if the thread info / thread was currently
   * executing something else this will lead to a lot of problems
   * USE WITH CARE, or better, don't use at all if you're a student
   * @param the ArchThreadInfo that we are going to mangle
   * @param start_function instruction pointer for the next instruction that gets executed
   */
  static void changeInstructionPointer(ArchThreadInfo *info, pointer function);

/**
 * creates the ArchThreadInfo for a user thread
 * @param info where the ArchThreadInfo is saved
 * @param start_function instruction pointer is set so start function
 * @param user_stack pointer to the userstack
 * @param kernel_stack pointer to the kernel stack
 */
  static void createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack);

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

/**
 * uninterruptable locked operation
 * exchanges value in variable lock with new_value and returns the old_value
 *
 * @param &lock Reference to variable being tested
 * @param new_value to set variable lock to
 * @returns old_value of variable lock
 */
  static uint32 testSetLock(uint32 &lock, uint32 new_value);

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

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose = true);
  static void printThreadRegisters(Thread *thread, bool verbose = true);
};

#endif
