#pragma once

#include "types.h"
#include "fs/FileSystemInfo.h"
#include "VgaColors.h"

#define STACK_CANARY ((uint32)0xDEADDEAD ^ (uint32)(size_t)this)

enum ThreadState
{
  Running, Sleeping, ToBeDestroyed
};

enum SystemState { BOOTING, RUNNING, KPANIC };
extern SystemState system_state;

class Thread;
struct ArchThreadRegisters;
class Loader;
class Terminal;
class Mutex;
class FsWorkingDirectory;
class Lock;

class Thread
{
    friend class Scheduler;
    friend class CpuLocalScheduler;
  public:

    static const char* threadStatePrintable[3];

    enum TYPE { KERNEL_THREAD, USER_THREAD };

    /**
     * Constructor with FsWorkingDirectory given
     * @param working_dir working directory informations for the new Thread
     * @param name Thread's name
     * @return Thread instance
     */
    Thread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type);

    virtual ~Thread();

    /**
     * Marks the thread to be deleted by the scheduler.
     * DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
     */
    virtual void kill();

    /**
     * runs whatever the user wants it to run;
     */
    virtual void Run() = 0;

    void* getKernelStackStartPointer();

    bool isStackCanaryOK();

    const char* getName() const;

    size_t getTID() const;

    Terminal* getTerminal();

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

    /**
     * Tells the scheduler if this thread is ready for scheduling
     * @return true if ready for scheduling
     */
    bool schedulable();

    bool canRunOnCpu(size_t cpu_id);

    bool isCurrentlyScheduled() const;
    bool isCurrentlyScheduledOnCpu(size_t cpu_id) const;

    void setSchedulingStartTimestamp(uint64 timestamp);
    uint64 schedulingStartTimestamp();


    uint32 kernel_stack_[2048];
    ArchThreadRegisters* kernel_registers_;
    ArchThreadRegisters* user_registers_;

    uint32 switch_to_userspace_;

    Loader* loader_;


    void setState(ThreadState state);

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

    volatile size_t currently_scheduled_on_cpu_ = -1;
    volatile size_t pinned_to_cpu = -1;

    CONSOLECOLOR console_color;

    bool* kprintfd_recursion_detected = nullptr;

  private:
    Thread(Thread const &src);
    Thread &operator=(Thread const &src);

    volatile ThreadState state_;

    size_t tid_;

    Terminal* my_terminal_;

  protected:
    ThreadState getState() const;

    FileSystemInfo* working_dir_;

    ustl::string name_;

    uint64 vruntime;
    uint64 sched_start;

    bool prev_schedulable = false;
    bool yielded = false;
};
