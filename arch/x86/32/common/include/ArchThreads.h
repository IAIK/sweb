/**
 * @file ArchThreads.h
 *
 */

#ifndef _ARCH_THREADS_H_
#define _ARCH_THREADS_H_

#include "types.h"

struct ArchThreadInfo
{
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  dpl;       // 64
  uint32  esp0;      // 68
  uint32  ss0;       // 72
  uint32  cr3;       // 76
  uint32  fpu[27];   // 80
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
 * not implemented
 *
 */
  static void switchToThreadOnIret(Thread *thread);

/**
 * deletes the info if not null
 *
 * @param info to be cleaned up
 *
 */
  static void cleanupThreadInfos(ArchThreadInfo *&info);

/**
 * creates the ArchThreadInfo for a kernel thread
 * @param info where the ArchThreadInfo is saved
 * @param start_function instruction pointer is set so start function
 * @param stack stackpointer
 */
  static void createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack);

 /**
  * initialises the ArchThreadInfo for an kernel thread into an already initialised thread info
  * @param the ArchThreadInfo we will overwrite
  * @param start_function instruction pointer is set so start function
  * @param stack stackpointer
  */
  static void initialseThreadInfosKernelThread(ArchThreadInfo *info, pointer start_function, pointer stack);


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
 * function to get the PageDirectory of a given thread
 *
 * @param *thread Pointer to Thread Object
 * @return returns pde page of *thread
 */
  static uint32 getPageDirectory(Thread *thread);

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

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, uint32 userspace_registers);
};

#endif
