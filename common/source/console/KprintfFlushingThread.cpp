#include "KprintfFlushingThread.h"

#include "Console.h"
#include "RingBuffer.h"
#include "Scheduler.h"
#include "Terminal.h"
#include "Thread.h"
#include "kprintf.h"

#include "ArchInterrupts.h"

void flushActiveConsole(RingBuffer<char>* rb)
{
    assert(main_console);
    assert(rb);
    assert(ArchInterrupts::testIFSet());
    char c = 0;
    while (rb->get(c))
    {
        main_console->getActiveTerminal()->write(c);
    }
    Scheduler::instance()->yield();
}

KprintfFlushingThread::KprintfFlushingThread(RingBuffer<char>* rb) :
    Thread(nullptr, "KprintfFlushingThread", Thread::KERNEL_THREAD),
    rb(rb)
{
}

void KprintfFlushingThread::Run()
{
    while (true)
    {
        flushActiveConsole(rb);
    }
}
