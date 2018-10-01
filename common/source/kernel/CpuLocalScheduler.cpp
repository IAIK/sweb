#include "CpuLocalScheduler.h"
#include "debug.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "backtrace.h"
#include "ustring.h"
#include "KernelMemoryManager.h"

CpuLocalScheduler::CpuLocalScheduler()
{
        addNewThread(&idle_thread_);
        addNewThread(&cleanup_thread_);
}

void CpuLocalScheduler::schedule()
{
        assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");
        if(!tryLockThreadList())
        {
                debug(CPU_SCHEDULER, "Scheduler thread list is currently locked. Abort scheduling\n");
                return;
        }

        auto it = threads_.begin();
        for(; it != threads_.end(); ++it)
        {
                if((*it)->schedulable())
                {
                        setCurrentThread(*it);
                        break;
                }
        }

        assert(it != threads_.end() && "No schedulable thread found");

        ustl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled

        unlockThreadList();

        if (currentThread()->switch_to_userspace_)
        {
                setCurrentThreadRegisters(currentThread()->user_registers_);
        }
        else
        {
                setCurrentThreadRegisters(currentThread()->kernel_registers_);
        }
}

void CpuLocalScheduler::addNewThread(Thread* thread)
{
        assert(thread);
        debug(SCHEDULER, "CPU %zu, addNewThread: %p  %zd:%s\n", ArchMulticore::getCpuID(), thread, thread->getTID(), thread->getName());
        if (getCurrentThread())
                ArchThreads::debugCheckNewThread(thread);
        KernelMemoryManager::instance()->getKMMLock().acquire();
        lockThreadList();
        KernelMemoryManager::instance()->getKMMLock().release(); // Could still be locked again by threads on other CPUs, but locking scheduling on this CPU is not a problem in this case
        threads_.push_back(thread);
        unlockThreadList();
}

void CpuLocalScheduler::cleanupDeadThreads()
{
        //debug(CPU_SCHEDULER, "CPU %zu, Cleaning up dead threads\n", ArchMulticore::getCpuID());
        lockThreadList();
        uint32 thread_count_max = threads_.size();
        if (thread_count_max > 1024)
                thread_count_max = 1024;
        Thread* destroy_list[thread_count_max];
        uint32 thread_count = 0;
        for (uint32 i = 0; i < threads_.size(); ++i)
        {
                Thread* tmp = threads_[i];
                if (tmp->getState() == ToBeDestroyed)
                {
                        destroy_list[thread_count++] = tmp;
                        threads_.erase(threads_.begin() + i); // Note: erase will not realloc!
                        --i;
                }
                if (thread_count >= thread_count_max)
                        break;
        }
        unlockThreadList();
        if (thread_count > 0)
        {
                for (uint32 i = 0; i < thread_count; ++i)
                {
                        delete destroy_list[i];
                }
                debug(SCHEDULER, "CPU %zu, cleanupDeadThreads: done\n", ArchMulticore::getCpuID());
        }
}

bool CpuLocalScheduler::isCurrentlyCleaningUp()
{
        return currentThread() == &cleanup_thread_;
}

Thread* CpuLocalScheduler::getCurrentThread()
{
        return currentThread_;
}

ArchThreadRegisters* CpuLocalScheduler::getCurrentThreadRegisters()
{
        return currentThreadRegisters_;
}


void CpuLocalScheduler::setCurrentThread(Thread* t)
{
        //debug(CPU_SCHEDULER, "CPU %zu, setCurrentThread: %p (%s)\n", ArchMulticore::getCpuID(), t, t->getName());
        currentThread_ = t;
}

void CpuLocalScheduler::setCurrentThreadRegisters(ArchThreadRegisters* r)
{
        //debug(CPU_SCHEDULER, "setCurrentThreadRegisters: %p\n", r);
        currentThreadRegisters_ = r;
}

void CpuLocalScheduler::lockThreadList()
{
        while(!tryLockThreadList());
}

bool CpuLocalScheduler::tryLockThreadList()
{
        return !ArchThreads::testSetLock(thread_list_lock_, 1);
}

void CpuLocalScheduler::unlockThreadList()
{
        thread_list_lock_ = 0;
}

size_t CpuLocalScheduler::getTicks()
{
        return ticks_;
}

void CpuLocalScheduler::incTicks()
{
        ++ticks_;
}
