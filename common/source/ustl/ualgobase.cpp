// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef NDEBUG  // Optimized code here. asserts slow it down, and are checked elsewhere.
#define NDEBUG
#endif

#include "ualgo.h"

namespace ustl {

// Generic version for implementing fill_nX_fast on non-i386 architectures.
template <typename T> static inline void stosv (T*& p, size_t n, T v)
    { while (n--) *p++ = v; }

#if __i386__ || __x86_64__

//----------------------------------------------------------------------
// Copy functions
//----------------------------------------------------------------------

static inline void movsb_dir_up (void) { asm volatile ("cld"); }
static inline void movsb_dir_down (void) { asm volatile ("std"); }

static inline void movsb (const void*& src, size_t nBytes, void*& dest)
{
    asm volatile ("rep;\n\tmovsb"
  : "=&S"(src), "=&D"(dest), "=&c"(nBytes)
  : "0"(src), "1"(dest), "2"(nBytes)
  : "memory");
}

static inline void movsd (const void*& src, size_t nWords, void*& dest)
{
    asm volatile ("rep;\n\tmovsl"
  : "=&S"(src), "=&D"(dest), "=&c"(nWords)
  : "0"(src), "1"(dest), "2"(nWords)
  : "memory");
}

template <> inline void stosv (uint8_t*& p, size_t n, uint8_t v)
{ asm volatile ("rep;\n\tstosb" : "=&D"(p), "=c"(n) : "0"(p), "1"(n), "a"(v) : "memory"); }
template <> inline void stosv (uint16_t*& p, size_t n, uint16_t v)
{ asm volatile ("rep;\n\tstosw" : "=&D"(p), "=c"(n) : "0"(p), "1"(n), "a"(v) : "memory"); }
template <> inline void stosv (uint32_t*& p, size_t n, uint32_t v)
{ asm volatile ("rep;\n\tstosl" : "=&D"(p), "=c"(n) : "0"(p), "1"(n), "a"(v) : "memory"); }


/// The fastest optimized backwards raw memory copy.
void copy_backward_fast (const void* first, const void* last, void* result) noexcept
{
    prefetch (first, 0, 0);
    prefetch (result, 1, 0);
    size_t nBytes (distance (first, last));
    movsb_dir_down();
    size_t nHeadBytes = uintptr_t(last) % 4;
    last = advance (last, -1);
    result = advance (result, -1);
    movsb (last, nHeadBytes, result);
    nBytes -= nHeadBytes;
    if (uintptr_t(result) % 4 == 3) {
  const size_t nMiddleBlocks = nBytes / 4;
  last = advance (last, -3);
  result = advance (result, -3);
  movsd (last, nMiddleBlocks, result);
  nBytes %= 4;
    }
    movsb (last, nBytes, result);
    movsb_dir_up();
}
#endif // __i386__

//----------------------------------------------------------------------
// Fill functions
//----------------------------------------------------------------------

void fill_n8_fast (uint8_t* dest, size_t count, uint8_t v) noexcept { memset (dest, v, count); }
void fill_n16_fast (uint16_t* dest, size_t count, uint16_t v) noexcept { stosv (dest, count, v); }
void fill_n32_fast (uint32_t* dest, size_t count, uint32_t v) noexcept { stosv (dest, count, v); }

/// Exchanges ranges [first, middle) and [middle, last)
void rotate_fast (void* first, void* middle, void* last) noexcept
{
    if (first == middle || middle == last)
  return;
    {
  char* f = (char*) first;
  char* m = (char*) middle;
  char* l = (char*) last;
  reverse (f, m);
  reverse (m, l);
  while (f != m && m != l)
      iter_swap (f++, --l);
  reverse (f, (f == m ? l : m));
    }
}

#if __GNUC__ < 4
size_t popcount (uint32_t v) noexcept
{
    const uint32_t w = v - ((v >> 1) & 0x55555555); // Algorithm from AMD optimization guide
    const uint32_t x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    return ((x + (x >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

#if HAVE_INT64_T
/// \brief Returns the number of 1s in \p v in binary.
size_t popcount (uint64_t v) noexcept
{
    v -= (v >> 1) & UINT64_C(0x5555555555555555);   // Algorithm from Wikipedia
    v = (v & UINT64_C(0x3333333333333333)) + ((v >> 2) & UINT64_C(0x3333333333333333));
    v = (v + (v >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
    return (v * UINT64_C(0x0101010101010101)) >> 56;
}
#endif  // HAVE_INT64_T
#endif  // !__GNUC__

//----------------------------------------------------------------------
// Miscellaneous instantiated stuff from headers which don't have enough
// to warrant creation of a separate file.cc
//----------------------------------------------------------------------

// Used in uspecial to print printable characters
const char _FmtPrtChr[2][8]={"'%c'","%d"};

} // namespace ustl
