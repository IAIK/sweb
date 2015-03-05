#ifndef _THREAD_H_
#define _THREAD_H_

#include "types.h"
#include "fs/FileSystemInfo.h"

#define STACK_CANARY (0xDEADDEAD)

enum ThreadState
{
  Running, Sleeping, ToBeDestroyed, Worker
};

enum SystemState { BOOTING, RUNNING, KPANIC };
extern SystemState system_state;

class Thread;
class ArchThreadInfo;
class Loader;
class Terminal;
class Mutex;
class FsWorkingDirectory;
class Lock;

extern Thread* currentThread;

class Thread
{
    friend class Scheduler;
  public:

    static const char* threadStatePrintable[4];

    /**
     * Constructor with FsWorkingDirectory given
     * @param working_dir working directory informations for the new Thread
     * @param name Thread's name
     * @return Thread instance
     */
    Thread(FileSystemInfo* working_dir, const char* name);

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

    pointer getStackStartPointer();

    Loader *loader_;

    ThreadState state_;

    const char *getName()
    {
      return name_.c_str();
    }

    size_t getTID()
    {
      return tid_;
    }

    Terminal *getTerminal();

    void setTerminal(Terminal *my_term);

    /**
     * getting the informations about the working Directory of this
     * Thread
     * @return the thread's FsWorkingDirectory
     */
    FileSystemInfo* getWorkingDirInfo(void);

    /**
     * sets the working directory informations of the this Thread
     * @param working_dir the new working directory informations
     */
    void setWorkingDirInfo(FileSystemInfo* working_dir);

    /**
     * prints a backtrace (i.e. the call stack) to the
     * debug output.
     * @param use_stored_thread_info determines whether to use the stored or the current thread registers
     */
    void printBacktrace();
    void printBacktrace(bool use_stored_registers);
    void printUserBacktrace();

    /**
     * jobs are only for worker threads
     */
    void addJob();
    void jobDone();
    void waitForNextJob();
    virtual bool hasWork();
    bool isWorker() const;

    /**
     * Tells the scheduler if this thread is ready for scheduling
     * @return true if ready for scheduling
     */
    bool schedulable();


	/**
	 * A part of the single-chained waiters list for the locks.
	 * It references to the next element of the list.
	 * In case of a spinlock it is a busy-waiter, else usually it is a sleeper ^^.
	 */
	Thread* next_thread_in_lock_waiters_list_;

	/**
	 * The information which lock the thread is currently waiting on.
	 */
	Lock* lock_waiting_on_;

	/**
	 * A single chained list containing all the locks held by the thread at the moment.
	 * This list is not locked. It may only be accessed by the thread himself,
	 * or by other threads in case they can ENSURE that this thread is not able to run at this moment.
	 * Changing the list has to be done atomic, else it cannot be ensured that the list is valid at any moment!
	 */
	Lock* holding_lock_list_;

  private:
    Thread(Thread const &src);
    Thread &operator=(Thread const &src);

    size_t num_jiffies_;
    size_t tid_;

    Terminal *my_terminal_;

  protected:
    FileSystemInfo* working_dir_;

    ustl::string name_;
    uint64 jobs_scheduled_;
    uint64 jobs_done_;

};

#endif
