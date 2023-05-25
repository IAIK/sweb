#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef assert
#define assert(cond) do { (cond) ? (void)0 : sweb_assert( #cond ,__LINE__,__FILE__, __PRETTY_FUNCTION__); } while (0)
#endif

    /**
     * called when assertion is used and true
     * @param condition assertion that has to be checked
     * @param line the function which to jump to
     * @param file where the assertion is called
     * @param function where the assertion is called
     */
    __attribute__((noreturn)) void sweb_assert(const char *condition, uint32_t line, const char* file, const char* function);


#ifdef __cplusplus
}
#endif

#if defined __USE_ISOC11 && !defined __cplusplus
# undef static_assert
# define static_assert _Static_assert
#endif
