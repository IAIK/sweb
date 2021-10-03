#include "KprintfFlushingThread.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "Console.h"
#include "Terminal.h"
#include "kprintf.h"
#include "RingBuffer.h"

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
    Thread(0, "KprintfFlushingThread", Thread::KERNEL_THREAD),
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
