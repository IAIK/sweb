#pragma once

#if defined(__FLT_EVAL_METHOD__)
    #if(__FLT_EVAL_METHOD__== 0)
        typedef float float_t;
        typedef double double_t;
    #elif(__FLT_EVAL_METHOD__ == 1)
        typedef double float_t;
        typedef double double_t;
    #elif(__FLT_EVAL_METHOD__ == 2)
        typedef long double float_t;
        typedef long double double_t;
    #endif
#else
    typedef float  float_t;
    typedef double double_t;
#endif

/* Value returned on overflow.  With IEEE 754 floating point, this is
   +Infinity, otherwise the largest representable positive value.  */
#define HUGE_VAL (__builtin_huge_val ())
/* This may provoke compiler warnings, and may not be rounded to
   +Infinity in all IEEE 754 rounding modes, but is the best that can
   be done in ISO C while remaining a constant expression.  10,000 is
   greater than the maximum (decimal) exponent for all supported
   floating-point formats and widths.  */

#define HUGE_VALF (__builtin_huge_valf ())
#define HUGE_VALL (__builtin_huge_vall ())

#define HUGE_VAL_F16 (__builtin_huge_valf16 ())
#define HUGE_VAL_F32 (__builtin_huge_valf32 ())
#define HUGE_VAL_F64 (__builtin_huge_valf64 ())
#define HUGE_VAL_F128 (__builtin_huge_valf128 ())
#define HUGE_VAL_F32X (__builtin_huge_valf32x ())
#define HUGE_VAL_F64X (__builtin_huge_valf64x ())
#define HUGE_VAL_F128X (__builtin_huge_valf128x ())

#define INFINITY (__builtin_inff ())

#define NAN (__builtin_nanf (""))

#define SNANF (__builtin_nansf (""))
#define SNAN (__builtin_nans (""))
#define SNANL (__builtin_nansl (""))
#define SNANF16 (__builtin_nansf16 (""))
#define SNANF32 (__builtin_nansf32 (""))
#define SNANF64 (__builtin_nansf64 (""))
#define SNANF128 (__builtin_nansf128 (""))
#define SNANF32X (__builtin_nansf32x (""))
#define SNANF64X (__builtin_nansf64x (""))
#define SNANF128X (__builtin_nansf128x (""))

#define M_E             2.7182818284590452354   /* e */
#define M_LOG2E         1.4426950408889634074   /* log_2 e */
#define M_LOG10E        0.43429448190325182765  /* log_10 e */
#define M_LN2           0.69314718055994530942  /* log_e 2 */
#define M_LN10          2.30258509299404568402  /* log_e 10 */
#define M_PI            3.14159265358979323846  /* pi */
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#define M_PI_4          0.78539816339744830962  /* pi/4 */
#define M_1_PI          0.31830988618379067154  /* 1/pi */
#define M_2_PI          0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI      1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2         1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2       0.70710678118654752440  /* 1/sqrt(2) */

/* float       ceilf( float arg ); */
/* double      ceil( double arg ); */
/* long double ceill( long double arg ); */
