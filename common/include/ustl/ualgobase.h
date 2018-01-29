// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once

#include "uutility.h"
#include <kstring.h>

namespace ustl {

#if HAVE_CPP11

template <typename T>
inline constexpr typename tm::RemoveReference<T>::Result&& move (T&& v) noexcept
    { return static_cast<typename tm::RemoveReference<T>::Result&&>(v); }

template <typename T>
inline constexpr T&& forward (typename tm::RemoveReference<T>::Result& v) noexcept
    { return static_cast<T&&>(v); }

template <typename T>
inline constexpr T&& forward (typename tm::RemoveReference<T>::Result&& v) noexcept
    { return static_cast<T&&>(v); }

template <typename Assignable> 
inline void swap (Assignable& a, Assignable& b)
{
    Assignable tmp = move(a);
    a = move(b);
    b = move(tmp);
}

#else

/// Assigns the contents of a to b and the contents of b to a.
/// This is used as a primitive operation by many other algorithms. 
/// \ingroup SwapAlgorithms
///
template <typename Assignable> 
inline void swap (Assignable& a, Assignable& b)
{
    Assignable tmp = a;
    a = b;
    b = tmp;
}

#endif

/// Equivalent to swap (*a, *b)
/// \ingroup SwapAlgorithms
///
template <typename Iterator> 
inline void iter_swap (Iterator a, Iterator b)
{
    swap (*a, *b);
}

/// Copy copies elements from the range [first, last) to the range
/// [result, result + (last - first)). That is, it performs the assignments
/// *result = *first, *(result + 1) = *(first + 1), and so on. [1] Generally,
/// for every integer n from 0 to last - first, copy performs the assignment
/// *(result + n) = *(first + n). Assignments are performed in forward order,
/// i.e. in order of increasing n. 
/// \ingroup MutatingAlgorithms
///
template <typename InputIterator, typename OutputIterator>
inline OutputIterator copy (InputIterator first, InputIterator last, OutputIterator result)
{
    for (; first != last; ++result, ++first)
  *result = *first;
    return result;
}

/// Copy_n copies elements from the range [first, first + n) to the range
/// [result, result + n). That is, it performs the assignments
/// *result = *first, *(result + 1) = *(first + 1), and so on. Generally,
/// for every integer i from 0 up to (but not including) n, copy_n performs
/// the assignment *(result + i) = *(first + i). Assignments are performed
/// in forward order, i.e. in order of increasing n.
/// \ingroup MutatingAlgorithms
///
template <typename InputIterator, typename OutputIterator>
inline OutputIterator copy_n (InputIterator first, size_t count, OutputIterator result)
{
    for (; count; --count, ++result, ++first)
  *result = *first;
    return result;
}

/// \brief Copy copies elements from the range (last, first] to result.
/// \ingroup MutatingAlgorithms
/// Copies elements starting at last, decrementing both last and result.
///
template <typename InputIterator, typename OutputIterator>
inline OutputIterator copy_backward (InputIterator first, InputIterator last, OutputIterator result)
{
    while (first != last)
  *--result = *--last;
    return result;
}

/// For_each applies the function object f to each element in the range
/// [first, last); f's return value, if any, is ignored. Applications are
/// performed in forward order, i.e. from first to last. For_each returns
/// the function object after it has been applied to each element.
/// \ingroup MutatingAlgorithms
///
template <typename InputIterator, typename UnaryFunction>
inline UnaryFunction for_each (InputIterator first, InputIterator last, UnaryFunction f)
{
    for (; first != last; ++first)
  f (*first);
    return f;
}

/// Fill assigns the value value to every element in the range [first, last).
/// That is, for every iterator i in [first, last),
/// it performs the assignment *i = value.
/// \ingroup GeneratorAlgorithms
///
template <typename ForwardIterator, typename T>
inline void fill (ForwardIterator first, ForwardIterator last, const T& value)
{
    for (; first != last; ++first)
  *first = value;
}

/// Fill_n assigns the value value to every element in the range
/// [first, first+count). That is, for every iterator i in [first, first+count),
/// it performs the assignment *i = value. The return value is first + count.
/// \ingroup GeneratorAlgorithms
///
template <typename OutputIterator, typename T>
inline OutputIterator fill_n (OutputIterator first, size_t count, const T& value)
{
    for (; count; --count, ++first)
  *first = value;
    return first;
}

#if CPU_HAS_MMX
extern "C" void copy_n_fast (const void* src, size_t count, void* dest) noexcept;
#else
inline void copy_n_fast (const void* src, size_t count, void* dest) noexcept
    { memcpy (dest, src, count); }
#endif
#if __i386__ || __x86_64__
extern "C" void copy_backward_fast (const void* first, const void* last, void* result) noexcept;
#else
inline void copy_backward_fast (const void* first, const void* last, void* result) noexcept
{
    const size_t nBytes (distance (first, last));
    memmove (advance (result, -nBytes), first, nBytes);
}
#endif
extern "C" void fill_n8_fast (uint8_t* dest, size_t count, uint8_t v) noexcept;
extern "C" void fill_n16_fast (uint16_t* dest, size_t count, uint16_t v) noexcept;
extern "C" void fill_n32_fast (uint32_t* dest, size_t count, uint32_t v) noexcept;
extern "C" void rotate_fast (void* first, void* middle, void* last) noexcept;

#if __GNUC__ >= 4
/// \brief Computes the number of 1 bits in a number.
/// \ingroup ConditionAlgorithms
inline size_t popcount (uint32_t v) { return __builtin_popcount (v); }
#if HAVE_INT64_T
inline size_t popcount (uint64_t v) { return __builtin_popcountll (v); }
#endif
#else
size_t popcount (uint32_t v) noexcept;
#if HAVE_INT64_T
size_t popcount (uint64_t v) noexcept;
#endif  // HAVE_INT64_T
#endif  // __GNUC__

//----------------------------------------------------------------------
// Optimized versions for standard types
//----------------------------------------------------------------------

#if WANT_UNROLLED_COPY

template <typename T>
inline T* unrolled_copy (const T* first, size_t count, T* result)
{
    copy_n_fast (first, count * sizeof(T), result);
    return advance (result, count);
}

template <>
inline uint8_t* copy_backward (const uint8_t* first, const uint8_t* last, uint8_t* result)
{
    copy_backward_fast (first, last, result);
    return result;
}

template <typename T>
inline T* unrolled_fill (T* result, size_t count, T value)
{
    for (; count; --count, ++result)
  *result = value;
    return result;
}
template <> inline uint8_t* unrolled_fill (uint8_t* result, size_t count, uint8_t value)
    { fill_n8_fast (result, count, value); return advance (result, count); }
template <> inline uint16_t* unrolled_fill (uint16_t* result, size_t count, uint16_t value)
    { fill_n16_fast (result, count, value); return advance (result, count); }
template <> inline uint32_t* unrolled_fill (uint32_t* result, size_t count, uint32_t value)
    { fill_n32_fast (result, count, value); return advance (result, count); }

#endif // WANT_UNROLLED_COPY

// Specializations for void* and char*, aliasing the above optimized versions.
//
// All these need duplication with const and non-const arguments, since
// otherwise the compiler will default to the unoptimized version for
// pointers not const in the caller's context, such as local variables.
// These are all inline, but they sure slow down compilation... :(
//
#define COPY_ALIAS_FUNC(ctype, type, alias_type)      \
template <> inline type* copy (ctype* first, ctype* last, type* result) \
{ return (type*) copy ((const alias_type*) first, (const alias_type*) last, (alias_type*) result); }
#if WANT_UNROLLED_COPY
#if HAVE_THREE_CHAR_TYPES
COPY_ALIAS_FUNC(const char, char, uint8_t)
COPY_ALIAS_FUNC(char, char, uint8_t)
#endif
COPY_ALIAS_FUNC(const int8_t, int8_t, uint8_t)
COPY_ALIAS_FUNC(int8_t, int8_t, uint8_t)
COPY_ALIAS_FUNC(uint8_t, uint8_t, uint8_t)
COPY_ALIAS_FUNC(const int16_t, int16_t, uint16_t)
COPY_ALIAS_FUNC(int16_t, int16_t, uint16_t)
COPY_ALIAS_FUNC(uint16_t, uint16_t, uint16_t)
#if CPU_HAS_MMX || (SIZE_OF_LONG > 4)
COPY_ALIAS_FUNC(const int32_t, int32_t, uint32_t)
COPY_ALIAS_FUNC(int32_t, int32_t, uint32_t)
COPY_ALIAS_FUNC(uint32_t, uint32_t, uint32_t)
#endif
#endif
COPY_ALIAS_FUNC(const void, void, uint8_t)
COPY_ALIAS_FUNC(void, void, uint8_t)
#undef COPY_ALIAS_FUNC
#define COPY_BACKWARD_ALIAS_FUNC(ctype, type, alias_type)       \
template <> inline type* copy_backward (ctype* first, ctype* last, type* result)  \
{ return (type*) copy_backward ((const alias_type*) first, (const alias_type*) last, (alias_type*) result); }
#if WANT_UNROLLED_COPY
#if HAVE_THREE_CHAR_TYPES
COPY_BACKWARD_ALIAS_FUNC(char, char, uint8_t)
#endif
COPY_BACKWARD_ALIAS_FUNC(uint8_t, uint8_t, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(int8_t, int8_t, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(uint16_t, uint16_t, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(const uint16_t, uint16_t, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(int16_t, int16_t, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(const int16_t, int16_t, uint8_t)
#endif
COPY_BACKWARD_ALIAS_FUNC(void, void, uint8_t)
COPY_BACKWARD_ALIAS_FUNC(const void, void, uint8_t)
#undef COPY_BACKWARD_ALIAS_FUNC
#define FILL_ALIAS_FUNC(type, alias_type, v_type)       \
template <> inline void fill (type* first, type* last, const v_type& value) \
{ fill ((alias_type*) first, (alias_type*) last, (const alias_type) value); }
FILL_ALIAS_FUNC(void, uint8_t, char)
FILL_ALIAS_FUNC(void, uint8_t, uint8_t)
#if WANT_UNROLLED_COPY
#if HAVE_THREE_CHAR_TYPES
FILL_ALIAS_FUNC(char, uint8_t, char)
FILL_ALIAS_FUNC(char, uint8_t, uint8_t)
#endif
FILL_ALIAS_FUNC(int8_t, uint8_t, int8_t)
FILL_ALIAS_FUNC(int16_t, uint16_t, int16_t)
#if CPU_HAS_MMX || (SIZE_OF_LONG > 4)
FILL_ALIAS_FUNC(int32_t, uint32_t, int32_t)
#endif
#endif
#undef FILL_ALIAS_FUNC
#define COPY_N_ALIAS_FUNC(ctype, type, alias_type)          \
template <> inline type* copy_n (ctype* first, size_t count, type* result)  \
{ return (type*) copy_n ((const alias_type*) first, count, (alias_type*) result); }
COPY_N_ALIAS_FUNC(const void, void, uint8_t)
COPY_N_ALIAS_FUNC(void, void, uint8_t)
#if WANT_UNROLLED_COPY
#if HAVE_THREE_CHAR_TYPES
COPY_N_ALIAS_FUNC(const char, char, uint8_t)
COPY_N_ALIAS_FUNC(char, char, uint8_t)
#endif
COPY_N_ALIAS_FUNC(int8_t, int8_t, uint8_t)
COPY_N_ALIAS_FUNC(uint8_t, uint8_t, uint8_t)
COPY_N_ALIAS_FUNC(const int8_t, int8_t, uint8_t)
COPY_N_ALIAS_FUNC(int16_t, int16_t, uint16_t)
COPY_N_ALIAS_FUNC(uint16_t, uint16_t, uint16_t)
COPY_N_ALIAS_FUNC(const int16_t, int16_t, uint16_t)
#if CPU_HAS_MMX || (SIZE_OF_LONG > 4)
COPY_N_ALIAS_FUNC(int32_t, int32_t, uint32_t)
COPY_N_ALIAS_FUNC(uint32_t, uint32_t, uint32_t)
COPY_N_ALIAS_FUNC(const int32_t, int32_t, uint32_t)
#endif
#endif
#undef COPY_N_ALIAS_FUNC
#define FILL_N_ALIAS_FUNC(type, alias_type, v_type)       \
template <> inline type* fill_n (type* first, size_t n, const v_type& value)  \
{ return (type*) fill_n ((alias_type*) first, n, (const alias_type) value); }
FILL_N_ALIAS_FUNC(void, uint8_t, char)
FILL_N_ALIAS_FUNC(void, uint8_t, uint8_t)
#if WANT_UNROLLED_COPY
#if HAVE_THREE_CHAR_TYPES
FILL_N_ALIAS_FUNC(char, uint8_t, char)
FILL_N_ALIAS_FUNC(char, uint8_t, uint8_t)
#endif
FILL_N_ALIAS_FUNC(int8_t, uint8_t, int8_t)
FILL_N_ALIAS_FUNC(int16_t, uint16_t, int16_t)
#if CPU_HAS_MMX || (SIZE_OF_LONG > 4)
FILL_N_ALIAS_FUNC(int32_t, uint32_t, int32_t)
#endif
#endif
#undef FILL_N_ALIAS_FUNC

extern const char _FmtPrtChr[2][8];

} // namespace ustl
