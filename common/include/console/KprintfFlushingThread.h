#pragma once

#include "Thread.h"
#include "RingBuffer.h"


class KprintfFlushingThread : public Thread
{
public:

    KprintfFlushingThread(RingBuffer<char>* rb);

    virtual void Run();
private:
    RingBuffer<char>* rb;
};
