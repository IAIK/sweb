#include "CpuLocalScheduler.h"
#include "debug.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "backtrace.h"
#include "ustring.h"

CpuLocalScheduler::CpuLocalScheduler() :
        thread_list_lock_(ustl::string("Cpu ") + ustl::to_string(ArchMulticore::getCpuID()) + ustl::string(" thread list lock"))
{
        addNewThread(&idle_thread_);
        addNewThread(&cleanup_thread_);
}

void CpuLocalScheduler::schedule()
{
        debug(CPU_SCHEDULER, "scheduling\n");
        assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");
        if(!thread_list_lock_.acquireNonBlocking(getCalledBefore(0), false))
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

        if (currentThread()->switch_to_userspace_)
        {
                setCurrentThreadRegisters(currentThread()->user_registers_);
        }
        else
        {
                setCurrentThreadRegisters(currentThread()->kernel_registers_);
        }

        debug(CPU_SCHEDULER, "Scheduling %s, user: %u\n", currentThread()->getName(), currentThread()->switch_to_userspace_);

        thread_list_lock_.release(getCalledBefore(0), false);
}

void CpuLocalScheduler::addNewThread(Thread* thread)
{
        assert(thread);
        debug(SCHEDULER, "CPU %zu, addNewThread: %p  %zd:%s\n", ArchMulticore::getCpuID(), thread, thread->getTID(), thread->getName());
        if (getCurrentThread())
                ArchThreads::debugCheckNewThread(thread);
        thread_list_lock_.acquire();
        threads_.push_back(thread);
        thread_list_lock_.release();
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
        debug(CPU_SCHEDULER, "setCurrentThread: %p (%s)\n", t, t->getName());
        currentThread_ = t;
}

void CpuLocalScheduler::setCurrentThreadRegisters(ArchThreadRegisters* r)
{
        debug(CPU_SCHEDULER, "setCurrentThreadRegisters: %p\n", r);
        currentThreadRegisters_ = r;
}
