#pragma once

#include "types.h"
#include "fs/FileSystemInfo.h"
#include "VgaColors.h"
#include "EASTL/unique_ptr.h"

#define STACK_CANARY ((uint32)0xDEADDEAD ^ (uint32)(size_t)this)



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

    enum ThreadState { Running, Sleeping, ToBeDestroyed };
    static constexpr const char* threadStatePrintable[3] = {
        "Running", "Sleeping", "ToBeDestroyed"
    };

    enum TYPE { KERNEL_THREAD, USER_THREAD };

    /**
     * Constructor with FsWorkingDirectory given
     * @param working_dir working directory informations for the new Thread
     * @param name Thread's name
     * @return Thread instance
     */
    Thread(FileSystemInfo* working_dir, eastl::string name, Thread::TYPE type);

    virtual ~Thread();

    Thread(const Thread& src) = delete;
    Thread& operator=(const Thread& src) = delete;

    /**
     * Marks the thread to be deleted by the CleanupThread.
     * DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
     */
    virtual void kill();

    /**
     * runs whatever the user wants it to run;
     * only used for kernel threads since user threads start in userspace
     */
    virtual void Run() = 0;

    [[nodiscard]] const char* getName() const;

    [[nodiscard]] size_t getTID() const;

    [[nodiscard]] Terminal* getTerminal() const;
    void setTerminal(Terminal* my_term);

    [[nodiscard]] FileSystemInfo* getWorkingDirInfo() const;
    void setWorkingDirInfo(FileSystemInfo* working_dir);

    [[nodiscard]] void* getKernelStackStartPointer() const;
    [[nodiscard]] bool isStackCanaryOK() const;


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

    [[nodiscard]] bool canRunOnCpu(size_t cpu_id) const;

    [[nodiscard]] bool isCurrentlyScheduled() const;
    [[nodiscard]] bool isCurrentlyScheduledOnCpu(size_t cpu_id) const;

    void setSchedulingStartTimestamp(uint64 timestamp);
    [[nodiscard]] uint64 schedulingStartTimestamp() const;

    // If you think you need to use this, you probably don't.
    // Ensuring proper synchronization here is not trivial
    void setState(ThreadState state);


    uint32 kernel_stack_[2048];
    eastl::unique_ptr<ArchThreadRegisters> kernel_registers_;
    eastl::unique_ptr<ArchThreadRegisters> user_registers_;

    uint32 switch_to_userspace_;

    Loader* loader_ = nullptr;

    /**
     * A part of the single-chained waiters list for the locks.
     * It references to the next element of the list.
     * In case of a spinlock it is a busy-waiter, else usually it is a sleeper ^^.
     */
    Thread* next_thread_in_lock_waiters_list_ = nullptr;

    /**
     * The information which lock the thread is currently waiting on.
     */
    Lock* lock_waiting_on_ = nullptr;

    /**
     * A single chained list containing all the locks held by the thread at the moment.
     * This list is not locked. It may only be accessed by the thread himself,
     * or by other threads in case they can ENSURE that this thread is not able to run at this moment.
     * Changing the list has to be done atomic, else it cannot be ensured that the list is valid at any moment!
     */
    Lock* holding_lock_list_ = nullptr;

    volatile size_t currently_scheduled_on_cpu_ = -1;
    volatile size_t pinned_to_cpu = -1;

    CONSOLECOLOR console_color;

    bool* kprintfd_recursion_detected = nullptr;

  private:
    void initKernelStackCanary();

    volatile ThreadState state_;

    size_t tid_;

    Terminal* my_terminal_ = nullptr;

  protected:
    // Only valid for currentThread (but why do you need to ask in this case?) or if
    // you can be sure the thread won't change state (e.g. a thread will never change out of the ToBeDestroyed state)
    [[nodiscard]] ThreadState getState() const;

    FileSystemInfo* working_dir_;

    eastl::string name_;

    uint64 vruntime = 0;
    uint64 sched_start = 0;

    bool prev_schedulable = false;
    bool yielded = false;
};
