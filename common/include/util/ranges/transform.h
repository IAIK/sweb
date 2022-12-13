#pragma once

#include "ranges.h"
#include "subrange.h"
#include "EASTL/functional.h"
#include "EASTL/utility.h"
#include "debug.h"

namespace ranges
{

    template<range V, typename F> class transform_view
    {
    private:
        using base_iterator_t = iterator_t<V>;
        using base_sentinel_t = sentinel_t<V>;

        // template<typename I>
        template<bool Const> struct iterator
        {
            using parent_type = eastl::
                conditional_t<Const, const transform_view<V, F>, transform_view<V, F>>;
            using base_type = eastl::conditional_t<Const, const V, V>;
            constexpr iterator() = default;

            constexpr iterator(parent_type& parent,
                               ranges::iterator_t<base_type> current) :
                parent_(eastl::addressof(parent)),
                current_(eastl::move(current))
            {
            }

            // iterator(I base_it, F func) :
            //     base_it_(eastl::forward<I>(base_it)),
            //     func_(eastl::forward<F>(func))
            // {
            // }

            constexpr iterator(iterator<!Const> i) requires Const
                && convertible_to<ranges::iterator_t<V>, ranges::iterator_t<base_type>>
                : parent_(eastl::move(i.parent_)), current_(eastl::move(i.current_))
            {
            }

            constexpr const ranges::iterator_t<base_type>& base() const & noexcept
            {
                return current_;
            }

            constexpr auto operator*() const
            {
                // debug(MAIN, "%s\n", __PRETTY_FUNCTION__);
                // auto x = *base_it_;
                // auto tx = eastl::invoke(func_, *base_it_);
                // debug(MAIN, "base val: %d, transformed: %d\n", x, tx);
                // return tx;
                // return eastl::invoke(func_, *base_it_);
                return eastl::invoke(parent_->func_, *current_);
            }

            iterator& operator++()
            {
                // debug(MAIN, "++transform it\n");
                ++current_;
                return *this;
            }

            iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            iterator& operator--()
            {
                --current_;
                return *this;
            }

            iterator operator--(int)
            {
                auto tmp = *this;
                --(*this);
                return tmp;
            }

            // template<typename IT, typename ST>
            friend constexpr bool operator==(iterator lhs, iterator rhs)
            {
                // debug(MAIN, "%s\n", __PRETTY_FUNCTION__);
                return lhs.current_ == rhs.current_;
            }

        private:
            parent_type* parent_;
            ranges::iterator_t<base_type> current_;
            // I base_it_;
            // F func_;
        };

        template<bool Const> struct sentinel
        {
            using parent_type = eastl::
                conditional<Const, const transform_view<V, F>, transform_view<V, F>>;
            using base_type = eastl::conditional<Const, const V, V>;

            sentinel() = default;

            constexpr explicit sentinel(ranges::sentinel_t<base_type> end) :
                end_(eastl::move(end))
            {
            }

            constexpr sentinel(sentinel<!Const> i) requires Const
                : end_(eastl::move(i.end_))
            {
            }

            constexpr ranges::sentinel_t<base_type> base() const { return end_; }

            friend constexpr bool operator==(const iterator<Const>& x, const sentinel& y)
            {
                return x.current_ == y.end_;
            }

            sentinel_t<base_type> end_;
        };

    public:
        transform_view(V base, F func) :
            base_(eastl::forward<V>(base)),
            func_(eastl::forward<F>(func))
        {
        }

        V base() const & requires copy_constructible<V> { return base_; }

        V base() && { return eastl::move(base_); }

        constexpr iterator<false> begin()
        {
            return iterator<false>{*this, ranges::begin(base_)};
        }

        constexpr iterator<true> begin() const requires ranges::range<const V>
        {
            return iterator<true>{*this, ranges::begin(base_)};
        }

        constexpr sentinel<false> end() { return sentinel<false>{ranges::end(base_)}; }

        constexpr iterator<false> end() requires ranges::common_range<V>
        {
            return iterator<false>{*this, ranges::end(base_)};
        }

        constexpr sentinel<true> end() const requires ranges::range<const V>
        {
            return sentinel<true>{ranges::end(base_)};
        }

        constexpr iterator<true> end() const requires ranges::common_range<const V>
        {
            return iterator<true>{*this, ranges::end(base_)};
        }

    private:
        V base_;
        F func_;
    };


    template<typename V, typename F>
    transform_view(V&& v, F f)->transform_view<ranges::all_t<V>, F>;
} // namespace ranges
