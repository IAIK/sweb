#pragma once

#include "EASTL/type_traits.h"
#include "EASTL/iterator.h"
#include "EASTL/memory.h"

namespace ranges
{


struct dangling
{
    constexpr dangling() noexcept = default;
    template<class... Args> constexpr dangling(Args&&...) noexcept {}
};

template<typename _Tp>
concept __member_begin = requires(_Tp& __t)
{
    __t.begin();
};

template<typename _Tp>
concept __adl_begin = requires(_Tp& __t)
{
    eastl::begin(__t);
};

template<typename _Tp>
requires eastl::is_array_v<_Tp> || __member_begin<_Tp&> || __adl_begin<_Tp&>
auto begin(_Tp& __t)
{
    // if constexpr (eastl::is_array_v<_Tp>)
    //     return __t + 0;
    // else
    if constexpr (__member_begin<_Tp&>)
        return __t.begin();
    else
        return eastl::begin(__t);
}

template<typename _Tp>
requires (eastl::is_array_v<_Tp> || __member_begin<_Tp&> || __adl_begin<_Tp&>)
auto end(_Tp& __t)
{
    // if constexpr (eastl::is_array_v<_Tp>)
    //     return __t + 0;
    // else
    if constexpr (__member_begin<_Tp&>)
        return __t.end();
    else
        return eastl::end(__t);
}

template<class T>
concept range = requires(T& t)
{
    ranges::begin(t);
    ranges::end(t);
};

template<range R> using iterator_t = decltype(ranges::begin(eastl::declval<R&>()));
template<range R> using sentinel_t = decltype(ranges::end(eastl::declval<R&>()));






template<typename I, typename S = I> class subrange
{
private:
    I iterator_;
    S sentinel_;

public:
    constexpr subrange(I iterator, S sentinel) :
        iterator_(eastl::move(iterator)),
        sentinel_(sentinel)
    {
    }

    template<range R>
    constexpr subrange(R&& range) :
        subrange(eastl::begin(range), eastl::end(range))
    {
    }

    constexpr auto begin() const { return iterator_; }

    constexpr auto end() const { return sentinel_; }

    constexpr bool empty() { return iterator_ == sentinel_; };

    // Enable structured decomposition
    template<size_t N>

    requires(N < 2) constexpr auto get() const
    {
        if constexpr (N == 0)
            return begin();
        else
            return end();
    }
};


template<class R> inline constexpr bool enable_borrowed_range = false;

template<class R>
concept borrowed_range = range<R> &&
    (eastl::is_lvalue_reference_v<R> || enable_borrowed_range<eastl::remove_cvref_t<R>>);


template<range R>
using borrowed_iterator_t = eastl::conditional_t<borrowed_range<R>,
                                               iterator_t<R>,
                                               dangling>;

template<range R>
using borrowed_subrange_t =
    eastl::conditional_t<borrowed_range<R>,
                       subrange<iterator_t<R>>,
                       dangling>;

template<borrowed_range R> subrange(R&& r) -> subrange<iterator_t<R>, sentinel_t<R>>;

template<class I, class S>
inline constexpr bool enable_borrowed_range<ranges::subrange<I, S>> = true;

namespace detail
{
    template<class T, class U>
    concept SameHelper = eastl::is_same_v<T, U>;
}

template<class T, class U>
concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

template<class T>
concept common_range =
    ranges::range<T> && same_as<ranges::iterator_t<T>, ranges::sentinel_t<T>>;

template<typename R> class owning_view
{
private:
    R r_;

public:
    owning_view() = default;
    owning_view(owning_view&& other) = default;
    constexpr owning_view(R&& t) :
        r_(eastl::move(t))
    {
    }
    owning_view(const owning_view&) = delete;
    owning_view& operator=(owning_view&& other) = default;
    owning_view& operator=(const owning_view&) = delete;

    constexpr R& base() & noexcept { return r_; }
    constexpr const R& base() const & noexcept { return r_; }
    constexpr R&& base() && noexcept { return eastl::move(r_); }
    constexpr const R&& base() const && noexcept { return eastl::move(r_); }

    constexpr iterator_t<R> begin() { return ranges::begin(r_); }
    constexpr auto begin() const requires range<const R> { return ranges::begin(r_); }

    constexpr sentinel_t<R> end() { return ranges::end(r_); }
    constexpr auto end() const requires range<const R> { return ranges::end(r_); }
    };

    template<typename R> owning_view(R&&) -> owning_view<R>;

    template<typename R>
    class ref_view
    {
    private:
        R* r_;
    public:
        ref_view() = default;
        ref_view(ref_view&& other) = default;
        constexpr ref_view(R&& t) :
            r_(eastl::addressof(static_cast<R&>(eastl::forward<R>(t))))
        {
        }
        ref_view(const ref_view&) = delete;
        ref_view& operator=(ref_view&& other) = default;
        ref_view& operator=(const ref_view&) = delete;

        constexpr R& base() const { return *r_; }

        constexpr iterator_t<R> begin() { return ranges::begin(*r_); }

        constexpr sentinel_t<R> end() { return ranges::end(*r_); }
    };

    template<class R> ref_view(R&) -> ref_view<R>;

    // template<class R>
    // using all = eastl::conditional_t<eastl::is_lvalue_reference_v<R>,
    //                                  ref_view<R>,
    //                                  owning_view<R>>;
    // using all = eastl::conditional_t<eastl::is_lvalue_reference_v<R>,
    //                                  decltype(ref_view{R})
    //                                  ref_view<decltype(eastl::declval<R>())>,
    //                                  owning_view<decltype(eastl::declval<R>())>>;

    template<typename _Range>
    concept __can_ref_view = requires
    {
        ref_view{eastl::declval<_Range>()};
    };

    template<typename _Range>
    concept __can_owning_view = requires
    {
        owning_view{eastl::declval<_Range>()};
    };

    struct all
    {
        template<range R> constexpr auto operator()(R&& r) const
        {
            if constexpr (__can_ref_view<R>)
            {
                return ref_view{eastl::forward<R>(r)};
            }
            else
            {
                return owning_view{eastl::forward<R>(r)};
            }
        }
    };

    template<class R>
    using all_t = decltype(ranges::all(eastl::declval<R>()));
} // namespace ranges

namespace std
{
    template<class I, class S>
    struct tuple_size<ranges::subrange<I, S>> : eastl::integral_constant<size_t, 2>
    {
    };

    template<class I, class S> struct tuple_element<0, ranges::subrange<I, S>>
    {
        using type = I;
    };

    template<class I, class S> struct tuple_element<0, const ranges::subrange<I, S>>
    {
        using type = I;
    };

    template<class I, class S> struct tuple_element<1, ranges::subrange<I, S>>
    {
        using type = S;
    };

    template<class I, class S> struct tuple_element<1, const ranges::subrange<I, S>>
    {
        using type = S;
    };
}; // namespace std

template<class From, class To>
concept convertible_to = eastl::is_convertible_v<From, To> && requires
{
    static_cast<To>(eastl::declval<From>());
};

template<class T>
concept destructible = eastl::is_nothrow_destructible_v<T>;

template<class T, class... Args>
concept constructible_from = destructible<T> && eastl::is_constructible_v<T, Args...>;

template<class T>
concept move_constructible = constructible_from<T, T> && convertible_to<T, T>;

template<class T>
concept copy_constructible = move_constructible<T> &&
    constructible_from<T, T&> && convertible_to<T&, T> &&
    constructible_from<T, const T&> && convertible_to<const T&, T> &&
    constructible_from<T, const T> && convertible_to<const T, T>;
