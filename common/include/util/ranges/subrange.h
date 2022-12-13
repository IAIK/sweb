#pragma once

#include <cstddef>
#include "ranges.h"
#include "EASTL/utility.h"
#include "EASTL/type_traits.h"


// namespace ranges
// {

//     template<typename I, typename S> class subrange
//     {
//     private:
//         I iterator_;
//         S sentinel_;

//     public:
//         constexpr subrange(I iterator, S sentinel) :
//             iterator_(eastl::move(iterator)),
//             sentinel_(sentinel)
//         {
//         }

//         template<range R>
//         constexpr subrange(R&& range) :
//             subrange(eastl::begin(range), eastl::end(range))
//         {
//         }

//         constexpr auto begin() const { return iterator_; }
//         constexpr auto end() const { return sentinel_; }

//         constexpr bool empty() { return iterator_ == sentinel_; };

//         // Enable structured decomposition
//         template<size_t N>
//         requires(N < 2)
//     constexpr auto get() const
//     {
//         if constexpr (N == 0)
//             return begin();
//         else
//             return end();
//     }
// };

// template<range R>
// subrange(R&& r) -> subrange<iterator_t<R>, sentinel_t<R>>;

// }

// namespace std
// {
//     template<class I, class S>
//     struct tuple_size<subrange<I, S>> : eastl::integral_constant<size_t, 2> { };


//     template<class I, class S> struct tuple_element<0, subrange<I, S>>
//     {
//         using type = I;
//     };

//     template<class I, class S> struct tuple_element<0, const subrange<I, S>>
//     {
//         using type = I;
//     };

//     template<class I, class S> struct tuple_element<1, subrange<I, S>>
//     {
//         using type = S;
//     };

//     template<class I, class S> struct tuple_element<1, const subrange<I, S>>
//     {
//         using type = S;
//     };
// };
