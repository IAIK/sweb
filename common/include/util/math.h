/**
 * @file math.h libc style math functions
 */

#ifndef MATH_H_
#define MATH_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * calculates the power of a number
 * this is very restricted from of the libc function
 * it does only allow natural exponents and integer
 * bases
 *
 * @param base
 * @param exponent
 *
 * @return the power
 */

uint32 pow(uint32 base, uint32 exponent);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H_ */
