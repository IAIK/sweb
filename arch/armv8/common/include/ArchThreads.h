#pragma once

#include "types.h"

#include "EASTL/unique_ptr.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

typedef struct
{
	size_t q1;
	size_t q2;
}NeonQ;

struct ArchThreadRegisters
{
	size_t X[31];
	NeonQ  Q[32];
	size_t SPSR;
	size_t ELR;
	size_t SP; // Saved previous stack pointer (SP_EL0 when coming from user mode, else SP)
	size_t TTBR0;
	size_t SP_SM; // Kernel stack pointer
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
 *
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
   * changes an existing ArchThreadRegisters so that execution will start / continue
   * at the function specified
   * it does not change anything else, and if the thread info / thread was currently
   * executing something else this will lead to a lot of problems
   * USE WITH CARE, or better, don't use at all if you're a student
   * @param the ArchThreadRegisters that we are going to mangle
   * @param start_function instruction pointer for the next instruction that gets executed
   */
  static void changeInstructionPointer(ArchThreadRegisters& info, void* function);

  static void* getInstructionPointer(ArchThreadRegisters& info);

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
 *
 * on x86: invokes int65, whose handler facilitates a task switch
 *
 */
  static void yield();

/**
 * sets a threads address space register to given address space
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
  static size_t testSetLock(size_t &lock, size_t new_value);

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
  static void atomic_set(size_t &target, size_t value);
  static void atomic_set(int32 &target, int32 value);
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
};
