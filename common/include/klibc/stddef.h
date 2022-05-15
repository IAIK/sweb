#pragma once

#ifdef __GNUG__
#define NULL __null
#elif defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void*)0)
#endif




#define offsetof(type, member)  __builtin_offsetof (type, member)


#ifdef __cplusplus
extern "C"
{
#endif

    typedef __SIZE_TYPE__ size_t;
    typedef __PTRDIFF_TYPE__ ptrdiff_t;
    typedef ptrdiff_t ssize_t; // Technically not defined in this header
#ifndef __cplusplus
    typedef __WCHAR_TYPE__ wchar_t;
#endif
    typedef decltype(nullptr) nullptr_t;

    typedef struct {
        long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
        // long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
        /* _Float128 is defined as a basic type, so max_align_t must be
           sufficiently aligned for it.  This code must work in C++, so we
           use __float128 here; that is only available on some
           architectures, but only on i386 is extra alignment needed for
           __float128.  */
#ifdef __i386__
        __float128 __max_align_f128 __attribute__((__aligned__(__alignof(__float128))));
#endif
    } max_align_t;

#ifdef __cplusplus
}
#endif
