/**
 * @file Thread.h
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#include "types.h"
#include "fs/FileSystemInfo.h"

enum ThreadState {Running, Sleeping, ToBeDestroyed};

class Thread;
class ArchThreadInfo;
class Loader;
class Terminal;

/**
 * @class Thread
 * thread base class
 */
class Thread
{
    friend class Scheduler;
  public:

    /**
     * Constructor
     * @return Thread instance
     */
    Thread();

    /**
     * Constructor with FileSystemInfo given
     * @return Thread instance
     */
    Thread ( FileSystemInfo *fs_info );

    /**
     * Destructor
     */
    virtual ~Thread();

    /**
     * Marks the thread to be deleted by the scheduler.
     * DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
     */
    void kill();

    /**
     * runs whatever the user wants it to run;
     */
    virtual void Run() =0;

    ArchThreadInfo *kernel_arch_thread_info_;
    ArchThreadInfo *user_arch_thread_info_;
    uint32 stack_[2048];

    uint32 switch_to_userspace_;

    /**
     * Returns the stack's start pointer.
     * @return the pointer
     */
    pointer getStackStartPointer();

    Loader *loader_;

    ThreadState state_;

    /**
     * Returns the thread's name.
     * @return the name
     */
    const char *getName()
    {
      if ( name_ )
        return name_;
      else
        return "<UNNAMED THREAD>";
    }

    uint32 getPID()
    {
      return pid_;
    }

    /**
     * Returns thread's current terminal
     * @return
     */
    Terminal *getTerminal();

    /**
     * Sets the thread's terminal
     * @param my_term the new terminal
     */
    void setTerminal ( Terminal *my_term );

    /**
     * returns the threads file system info
     * @return the threads file system info
     */
    FileSystemInfo *getFSInfo();

    /**
     * sets the threads file system info
     * @param fs_info the filesystem info to set
     */
    void setFSInfo ( FileSystemInfo *fs_info );

  private:

    /**
     * Copy Constructor (not implemented)
     * @param src the object to copy
     * @return the new object
     */
    Thread ( Thread const &src );

    /**
     * Operator = using Copy Constructor (not implemented)
     * @param src the object to copy
     * @return the new object
     */
    Thread &operator= ( Thread const &src );

    uint64 num_jiffies_;
    uint32 pid_;

    Terminal *my_terminal_;

  protected:
    FileSystemInfo *fs_info_;
    char *name_;
};









#endif
