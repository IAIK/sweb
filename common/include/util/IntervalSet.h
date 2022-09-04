#pragma once
#include "EASTL/utility.h"
#include "EASTL/map.h"


template <typename T, typename TAllocator = EASTLAllocatorType>
class IntervalSet : public eastl::map<T, T, eastl::less<T>, TAllocator>
{
public:
    typedef eastl::map<T, T, eastl::less<T>, TAllocator>                       base_type;
    typedef IntervalSet<T, TAllocator>                                         this_type;
    typedef TAllocator                                                         allocator_type;
    typedef typename base_type::iterator                                       iterator;
    typedef typename base_type::key_type                                       key_type;
    typedef typename base_type::mapped_type                                    mapped_type;

    using base_type::begin;
    using base_type::end;
    using base_type::get_allocator;
    using base_type::upper_bound;

    using Interval = eastl::pair<T, T>;

    explicit IntervalSet(const allocator_type& allocator = allocator_type("IntervalSet"))
        : base_type(allocator)
    {
    }

    eastl::pair<iterator, bool> insert(const Interval& interval)
    {
        assert(eastl::less_equal{}(interval.first, interval.second));

        iterator interval_it;
        iterator next_it = upper_bound(interval.first);

        if (next_it == begin() || // is new lowest element
            eastl::less{}(eastl::prev(next_it)->second, interval.first)) // new interval starts after previous end
        {
            // insert new element before next_it
            interval_it = base_type::insert(next_it, interval);
        }
        else
        {
            // Starts inside previous interval
            iterator prev_it = eastl::prev(next_it);
            interval_it = prev_it;
            if (eastl::equal_to{}(prev_it->second, interval.second))
            {
                // fully contained
                return {prev_it, false};
            }
            else
            {
                // extend end
                interval_it->second = interval.second;
            }
        }

        // merge forward
        while (next_it != end() && eastl::greater_equal{}(interval.second, next_it->first))
        {
            interval_it->second = eastl::max(next_it->second, interval_it->second);
            next_it = base_type::erase(next_it);
        }

        return {interval_it, true};
    }

    iterator erase(const Interval& interval)
    {
        assert(eastl::less_equal{}(interval.first, interval.second));

        auto next_it = base_type::lower_bound(interval.first);

        if (next_it != begin())
        {
            auto prev_it = eastl::prev(next_it);

            if (eastl::less{}(prev_it->first, interval.first) &&
                eastl::less{}(interval.first, prev_it->second))
            {
                // interval starts in prev
                prev_it->second = interval.first;
            }
        }

        while (next_it != end() && eastl::less{}(next_it->first, interval.second))
        {
            if (eastl::less_equal{}(next_it->second, interval.second))
            {
                // completely overlapping
                next_it = base_type::erase(next_it);
            }
            else
            {
                // partially overlapping, increase start
                Interval tmp_val = eastl::move(*next_it);
                next_it = base_type::erase(next_it);

                tmp_val.first = interval.second;
                base_type::emplace_hint(next_it, eastl::move(tmp_val));
            }
        }

        return next_it;
    }

private:
};
