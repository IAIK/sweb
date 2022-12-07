#pragma once

#include "EASTL/iterator.h"

template<typename T> class reverse_view
{
public:
    reverse_view(const T& source_view) :
        source_view_(source_view)
    {
    }

    auto begin()
    {
        if constexpr (requires { source_view_.rbegin(); })
        {
            return base().rbegin();
        }
        else
        {
            auto it = base().begin();
            auto end_it = base().end();
            while (it != end_it)
            {
                ++it;
            }
            return eastl::reverse_iterator(it);
        }
    }

    auto end()
    {
        if constexpr (requires { base().rend(); })
        {
            return source_view_.rend();
        }
        else
        {
            return eastl::reverse_iterator(base().begin());
        }
    }

    auto base()
    {
        return source_view_;
    }

private:
    T source_view_;
};
