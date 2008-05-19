
#include "string.h"

size_t strlcpy ( char* dest, const char* src, size_t size )
{
  const char* src_start = src;
  char *dst_iter = dest;
  size_t n = size;

  if ( n > 1 )
  {
    --n;

    while ( n-- )
    {
      if ( ( *dst_iter++ = *src++ ) == 0 )
      {
        break;
      }
    }
  }

  // terminate dest, if it was not done already
  *dst_iter = '\0';

  while ( *src )
  {
    ++src;
  }

  return ( src - src_start );
}

