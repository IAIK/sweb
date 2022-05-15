#pragma once

#define STRING_SAVE

#include "types.h"
#include "paging-definitions.h"
#include <cstring>

/**
 * Convert a define to a string.
 * This macro is used to fi. resolve defined numbers within a constant string.
 * @param _s the macro to convert to a string
 * @return the string which is resolved by the macro
 */
#ifndef macroToString
#define _macroToString(_s) #_s
#define macroToString(_s) _macroToString(_s)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  void *memccpy(void *dest, const void *src, uint8 c, size_t length);
  char *strdup(const char *src);
  size_t strlcat(char *dest, const char*append, size_t size);
  void *memnotchr(const void *block, uint8 c, size_t size);
  void *memrchr(const void *block, uint8 c, size_t size);
  char* itoa(int value, char * str, int base);

  /**
   * copy length bytes from src to dest, even if they overlap
   */
  void bcopy(void *src, void* dest, size_t length);

  /**
   * Compare size bytes of region1 and region2
   * @return the difference between the first differing bytes or zero on equality
   */
  int32 bcmp(const void *region1, const void *region2, size_t size);

  /**
   * Computes a CRC- like checksum
   */
  uint32 checksum(const uint32* src, uint32 nbytes);

#ifdef __cplusplus
}
#endif
