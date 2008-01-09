/**
 * @file ArchThreads.h
 *
 */

#ifndef _ARCH_THREADS_H_
#define _ARCH_THREADS_H_

#include "types.h"

class ArchThreadInfo;
class Thread;

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
 * sets a threads PageDirectory to the one in page_dir_physical_page
 *
 * @param *thread Pointer to Thread Object
 * @param page_dir_physical_page The Page where a valid pde can be found
 */
  static void setPageDirectory(Thread *thread, uint32 page_dir_physical_page);

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
