#pragma once

# if __INT_WIDTH__ == 32
  #ifndef __PRI_32_PREFIX
    #define __PRI_32_PREFIX ""
  #endif
#endif

#if __LONG_WIDTH__ == 32
  #ifndef __PRI_32_PREFIX
    #define __PRI_32_PREFIX "l"
  #endif
#elif __LONG_WIDTH__ == 64
  #ifndef __PRI_64_PREFIX
    #define __PRI_64_PREFIX "l"
  #endif
#else
  #error "unexpected sizeof long"
#endif

// g++ defines __LONG_LONG_WIDTH__, clang defines __LLONG_WIDTH__
#if __LONG_LONG_WIDTH__ == 64 || __LLONG_WIDTH__ == 64
  #ifndef __PRI_64_PREFIX
    #define __PRI_64_PREFIX "ll"
  #endif
#else
  #error "unexpected sizeof long long"
#endif

#if !defined(__PRI_32_PREFIX) || !defined(__PRI_64_PREFIX)
  #error undefined integer format prefix
#endif

#define PRIx32 __PRI_32_PREFIX "x"
#define PRId32 __PRI_32_PREFIX "d"
#define PRIi32 __PRI_32_PREFIX "i"
#define PRIu32 __PRI_32_PREFIX "u"

#define PRIx64 __PRI_64_PREFIX "x"
#define PRId64 __PRI_64_PREFIX "d"
#define PRIi64 __PRI_64_PREFIX "i"
#define PRIu64 __PRI_64_PREFIX "u"
