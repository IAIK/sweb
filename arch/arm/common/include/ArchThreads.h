#pragma once

#include "SpinLock.h"

#include "types.h"

#include "EASTL/unique_ptr.h"

extern SpinLock global_atomic_add_lock;

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadRegisters
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

extern "C" void memory_barrier();


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
                                                                      void* stack);

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
   * @param the ArchThreadRegisters that we are going to mangle
   * @param start_function instruction pointer for the next instruction that gets executed
   */
  static void changeInstructionPointer(ArchThreadRegisters& info, void* function);

  static void* getInstructionPointer(ArchThreadRegisters& info);



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
  template <typename T>
  static T testSetLock(T& lock, T new_value)
  {
      if constexpr (__atomic_always_lock_free(sizeof(T), 0))
      {
          return  __atomic_exchange_n(&lock, new_value, __ATOMIC_SEQ_CST);
      }
      else
      {
          T result;
          memory_barrier();
          asm("swp %[r], %[n], [%[l]]" : [r]"=&r"(result) : [n]"r"(new_value), [l]"r"(&lock));
          memory_barrier();
          return result;
      }
  }

  /**
   * Counterpart to testSetLock()
   * Writes 0 to the lock variable and provides a memory release barrier
   * (ensures all previous memory stores are visible)
   */
  template<typename T>
  static void syncLockRelease(volatile T &lock)
  {
      if constexpr (__atomic_always_lock_free(sizeof(T), 0))
      {
          __sync_lock_release(&lock);
      }
      else
      {
          lock = 0;
          memory_barrier();
      }
  }

/**
 * atomically increments or decrements value by increment
 *
 * @param &value Reference to value
 * @param increment can be positive or negative
 * @returns old value of value
 */
  template<typename T>
  static T atomic_add(T &value, T increment)
  {
      if constexpr (__atomic_always_lock_free(sizeof(T), 0))
      {
          return __sync_fetch_and_add(&value, increment);
      }
      else
      {
          global_atomic_add_lock.acquire();
          T result = value;
          value += increment;
          global_atomic_add_lock.release();
          return result;
      }
  }

  /**
   * Atomically set a target to another value.
   *
   * @param target The target which shall be set
   * @param value The value which shall be set
   */
  template<typename T>
  static void atomic_set(T& target, T value)
  {
      if constexpr (__atomic_always_lock_free(sizeof(T), 0))
      {
          __atomic_store_n (&target, value, __ATOMIC_SEQ_CST);
      }
      else
      {
          // just re-use the method for exchange. Under ARM the build-ins do not work...
          testSetLock(target, value);
      }
  }

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
