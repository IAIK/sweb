/**
 * @file ArchThreads.h
 *
 */

#ifndef _ARCH_THREADS_H_
#define _ARCH_THREADS_H_

#include "types.h"

struct ArchThreadInfo
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
  uint64  dpl;       // 192
  uint64  rsp0;      // 200
  uint64  ss0;       // 208
  uint64  cr3;       // 216
  uint32  fpu[28];   // 224
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
 * sets a threads page map level 4
 *
 * @param *thread Pointer to Thread Object
 * @param arch_memory the arch memory object for the address space
 */
  static void setAddressSpace(Thread *thread, ArchMemory& arch_memory);

/**
 * function to get the PageDirectory of a given thread
 *
 * @param *thread Pointer to Thread Object
 * @return returns pde page of *thread
 */
  static uint32 getPageDirPointerTable(Thread *thread);

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
