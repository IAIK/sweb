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
     * Constructor for a new thread with a given working directory, name and type
     * @param working_dir working directory information for the new Thread
     * @param name The name of the thread
     * @param type The type of the thread (user or kernel thread)
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

    /**
     * Get a pointer to the start (highest address) of the kernel stack
     */
    [[nodiscard]] void* getKernelStackStartPointer() const;

    [[nodiscard]] bool isStackCanaryOK() const;
    [[nodiscard]] static bool currentThreadIsStackCanaryOK();


    /**
     * Prints a backtrace (i.e. the call stack) to the debug output.
     */
    void printBacktrace();
    /**
     * Prints a backtrace (i.e. the call stack) to the debug output.
     * @param use_stored_registers whether to use the stored or the current thread registers
     */
    void printBacktrace(bool use_stored_registers);

    /**
     * Tells the scheduler if this thread can be scheduled
     * @return true if ready for scheduling
     */
    bool schedulable();

    /**
     * Check if the thread is allowed to run on the given CPU
     *
     * @param cpu_id id of the CPU
     * @return true if the thread is allowed to run on the given CPU, false otherwise
     */
    [[nodiscard]] bool canRunOnCpu(size_t cpu_id) const;

    /**
     * Check if the thread is currently scheduled on any CPU
     */
    [[nodiscard]] bool isCurrentlyScheduled() const;
    /**
     * Check if the thread is currently scheduled on the given CPU
     */
    [[nodiscard]] bool isCurrentlyScheduledOnCpu(size_t cpu_id) const;

    /**
     * Set the scheduling start timestamp. Used by the scheduler to track thread runtime.
     */
    void setSchedulingStartTimestamp(uint64 timestamp);
    /**
     * Get the scheduling start timestamp.
     */
    [[nodiscard]] uint64 schedulingStartTimestamp() const;

    // If you think you need to use this, you probably don't.
    // Ensuring proper synchronization here is not trivial and chances are you'll introduce a race condition.
    // This function is intended for internal low level use by locking mechanisms.
    // Use the provided higher level mechanisms instead (e.g. Condition, Mutex, ...)
    void setState(ThreadState state);


    uint32 kernel_stack_[2048]; // Stack used by the thread while it is running in the kernel

    eastl::unique_ptr<ArchThreadRegisters> kernel_registers_; // Stores thread context for user mode
    eastl::unique_ptr<ArchThreadRegisters> user_registers_; // Stores thread context for kernel mode

    uint32 switch_to_userspace_; // Whether the thread is running in userspace or in kernel (determines which register set is used on context switch)

    Loader* loader_ = nullptr;

    /**
     * A part of the single-chained waiters list for the locks.
     * It references to the next element of the list.
     */
    Thread* next_thread_in_lock_waiters_list_ = nullptr;

    /**
     * Pointer to the lock the thread is currently waiting on (if any)
     */
    Lock* lock_waiting_on_ = nullptr;

    /**
     * A single chained list containing all the locks held by the thread at the moment.
     * This list is not locked. It may only safely be accessed by the thread itself,
     * or by other threads if they can ENSURE that this thread is not able to run at this moment.
     * Changing the list has to be done atomic as it needs to be valid at all times!
     */
    Lock* holding_lock_list_ = nullptr;

    volatile size_t currently_scheduled_on_cpu_ = -1; // Id of the CPU the thread is currently scheduled on (-1 if not scheduled)
    volatile size_t pinned_to_cpu = -1; // Id of the CPU the thread is pinned to (-1 if allowed to run on all CPUs)

    CONSOLECOLOR console_color; // Color used for the debug spinner background at the top left of the SWEB console window while the thread is running

    bool* kprintfd_recursion_detected = nullptr; // Used detect recursive calls to kprintfd and prevent deadlocks (e.g. pagefault inside kprintfd)

private:
    void initKernelStackCanary();

    volatile ThreadState state_; // Run state of the thread. Can change at any time.

    size_t tid_; // Thread id

    Terminal* my_terminal_ = nullptr;

protected:
    // Only valid for currentThread (but why do you need to ask in this case?) or if
    // you can be sure the thread won't change state (e.g. a thread will never change out of the ToBeDestroyed state)
    [[nodiscard]] ThreadState getState() const;

    FileSystemInfo* working_dir_;

    eastl::string name_;

    /**
     * Virtual runtime of the thread. Used by scheduler to determine which thread to schedule next.
     * Does **NOT** correspond to the actual time a thread has been running. Only useful for the scheduler itself.
     */
    uint64 vruntime = 0;
    /**
     * Timestamp when the thread has been scheduled. Used by scheduler to track thread runtime.
     */
    uint64 sched_start = 0;

    bool prev_schedulable = false; // Used by scheduler to ensure correct timeslot distribution when a thread is blocked and cannot be scheduled
    bool yielded = false; // Used by scheduler to adjust thread vruntime and prioritize other threads if a thread yields its timeslot
};
