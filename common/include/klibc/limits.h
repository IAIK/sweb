#pragma once

/* Copyright (C) 1991-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/*
 *      ISO C99 Standard: 7.10/5.2.4.2.1 Sizes of integer types <limits.h>
 */

#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_ 1


/* Maximum length of any multibyte character in any locale.
   We define this value here since the gcc header does not define
   the correct value.  */
#define MB_LEN_MAX      16


/* These assume 8-bit `char's, 16-bit `short int's,
   and 32-bit `int's and `long int's.  */

/* Number of bits in a `char'.  */
#  define CHAR_BIT      8

/* Minimum and maximum values a `signed char' can hold.  */
#  define SCHAR_MIN     (-128)
#  define SCHAR_MAX     127

/* Maximum value an `unsigned char' can hold.  (Minimum is 0.)  */
#  define UCHAR_MAX     255

/* Minimum and maximum values a `char' can hold.  */
#  ifdef __CHAR_UNSIGNED__
#   define CHAR_MIN     0
#   define CHAR_MAX     UCHAR_MAX
#  else
#   define CHAR_MIN     SCHAR_MIN
#   define CHAR_MAX     SCHAR_MAX
#  endif

/* Minimum and maximum values a `signed short int' can hold.  */
#  define SHRT_MIN      (-32768)
#  define SHRT_MAX      32767

/* Maximum value an `unsigned short int' can hold.  (Minimum is 0.)  */
#  define USHRT_MAX     65535

/* Minimum and maximum values a `signed int' can hold.  */
#  define INT_MIN       (-INT_MAX - 1)
#  define INT_MAX       2147483647

/* Maximum value an `unsigned int' can hold.  (Minimum is 0.)  */
#  define UINT_MAX      4294967295U

/* Minimum and maximum values a `signed long int' can hold.  */
#  if __WORDSIZE == 64
#   define LONG_MAX     9223372036854775807L
#  else
#   define LONG_MAX     2147483647L
#  endif
#  define LONG_MIN      (-LONG_MAX - 1L)

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0.)  */
#  if __WORDSIZE == 64
#   define ULONG_MAX    18446744073709551615UL
#  else
#   define ULONG_MAX    4294967295UL
#  endif


/* The <limits.h> files in some gcc versions don't define LLONG_MIN,
   LLONG_MAX, and ULLONG_MAX.  Instead only the values gcc defined for
   ages are available.  */
# ifndef LLONG_MIN
#  define LLONG_MIN     (-LLONG_MAX-1)
# endif
# ifndef LLONG_MAX
#  define LLONG_MAX     __LONG_LONG_MAX__
# endif
# ifndef ULLONG_MAX
#  define ULLONG_MAX    (LLONG_MAX * 2ULL + 1)
# endif

/* The integer width macros are not defined by GCC's <limits.h> before
   GCC 7, or if _GNU_SOURCE rather than
   __STDC_WANT_IEC_60559_BFP_EXT__ is used to enable this feature.  */
# ifndef CHAR_WIDTH
#  define CHAR_WIDTH 8
# endif
# ifndef SCHAR_WIDTH
#  define SCHAR_WIDTH 8
# endif
# ifndef UCHAR_WIDTH
#  define UCHAR_WIDTH 8
# endif
# ifndef SHRT_WIDTH
#  define SHRT_WIDTH 16
# endif
# ifndef USHRT_WIDTH
#  define USHRT_WIDTH 16
# endif
# ifndef INT_WIDTH
#  define INT_WIDTH 32
# endif
# ifndef UINT_WIDTH
#  define UINT_WIDTH 32
# endif
# ifndef LONG_WIDTH
#  define LONG_WIDTH __WORDSIZE
# endif
# ifndef ULONG_WIDTH
#  define ULONG_WIDTH __WORDSIZE
# endif
# ifndef LLONG_WIDTH
#  define LLONG_WIDTH 64
# endif
# ifndef ULLONG_WIDTH
#  define ULLONG_WIDTH 64
# endif

#endif
