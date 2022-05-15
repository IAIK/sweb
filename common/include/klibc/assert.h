#pragma once

#include "stdint.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define assert(cond) do { (cond) ? (void)0 : sweb_assert( #cond ,__LINE__,__FILE__); } while (0)

/**
 * called when assertion is used and true
 * @param condition assertion that has to be checked
 * @param line the function which to jump to
 * @param file where the assertion is called
 */
    __attribute__((noreturn)) void sweb_assert(const char *condition, uint32_t line, const char* file);


#ifdef __cplusplus
}
#endif

#if defined __USE_ISOC11 && !defined __cplusplus
# undef static_assert
# define static_assert _Static_assert
#endif
