#pragma once

#include "EASTL/atomic.h"
#include "debug.h"
#include "assert.h"

// Multiple producer, single consumer lock free FIFO queue
// Node class T needs to have a eastl::atomic<T*> next_node_ member
template<class T>
class NonBlockingQueue
{
public:
    void pushFront(T* node)
    {
        T* expected_next = list_head_.load();
        do
        {
            node->next_node_ = expected_next;
        }
        while (!list_head_.compare_exchange_weak(expected_next, node)); // compare exchange loads expected_next with the current value on failure
    }

    T* peekBack()
    {
        eastl::atomic<T*>* curr_ptr = &list_head_;

        T* curr = curr_ptr->load();
        if (!curr)
        {
            return curr;
        }

        while (curr->next_node_)
        {
            curr_ptr = &curr->next_node_;
            curr = curr->next_node_;
        }

        return curr;
    }

    T* popBack()
    {
        eastl::atomic<T*>* curr_ptr = &list_head_;

        T* curr = curr_ptr->load();
        if (!curr)
        {
            return curr;
        }

        while (curr->next_node_.load())
        {
            curr_ptr = &curr->next_node_;
            curr = curr->next_node_;

            assert(curr);
        }

        T* last_node = curr;

        // list head might change if new nodes are added (but last node won't), so we need to be prepared to walk the list forward until we find the last node again
        while (!curr_ptr->compare_exchange_strong(curr, nullptr))
        {
            if (curr != nullptr)
            {
                curr_ptr = &curr->next_node_;
            }

            curr = last_node;
        }

        return last_node;
    }

private:
    eastl::atomic<T*> list_head_ = nullptr;
};
