#pragma once

#include "RingBuffer.h"
#include "Thread.h"

class KprintfFlushingThread : public Thread
{
public:
    KprintfFlushingThread(RingBuffer<char>* rb);

    void Run() override;

private:
    RingBuffer<char>* rb;
};
