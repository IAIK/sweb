#pragma once

#include "EASTL/atomic.h"

// Atomic multiple producer single consumer queue
template <typename T>
class AtomicMpScQueue
{
public:
    void pushFront(T* item)
    {
        T* expected_next = nullptr;
        do
        {
            item->next = expected_next;
        } while(!queue_head.compare_exchange_weak(expected_next, item));
    }

    T* takeAll()
    {
        T* front = queue_head.exchange(nullptr);
        return front;
    }

private:
    eastl::atomic<T*> queue_head = nullptr;
};
