#pragma once

#if __LONG_WIDTH__ == 32
  #define __PRI_32_PREFIX "l"
  #define __PRI_64_PREFIX "ll"
#elif __LONG_WIDTH__ == 64
  #define __PRI_32_PREFIX ""
  #define __PRI_64_PREFIX "l"
#else
  #error "invalid sizeof long"
#endif

#define PRIx32 __PRI_32_PREFIX "x"
#define PRId32 __PRI_32_PREFIX "d"
#define PRIi32 __PRI_32_PREFIX "i"
#define PRIu32 __PRI_32_PREFIX "u"

#define PRIx64 __PRI_64_PREFIX "x"
#define PRId64 __PRI_64_PREFIX "d"
#define PRIi64 __PRI_64_PREFIX "i"
#define PRIu64 __PRI_64_PREFIX "u"
