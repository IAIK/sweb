/**
 * @file assert.h
 *
 */
 
#ifndef ASSSERT_H__
#define ASSSERT_H__

#include "types.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define assert(cond) (cond) ? (void)0 : sweb_assert( #cond ,__LINE__,__FILE__);
#define prenew_assert(condition) pre_new_sweb_assert(condition,__LINE__,__FILE__);

/** 
 * called when assertion is used and true
 * @param condition assertion that has to be checked
 * @param line the function which to jump to
 * @param file where the assertion is called
 */
void sweb_assert(const char *condition, uint32 line, const char* file);

/** 
 * called when prenew assertion is used
 * @param condition assertion that has to be checked
 * @param line the function which to jump to
 * @param file where the assertion is called
 */
void pre_new_sweb_assert(uint32 condition, uint32 line, char* file);

#ifdef __cplusplus
}
#endif

#endif
