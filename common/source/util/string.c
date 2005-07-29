/// @file string.c basic string and array manipulation
/// @author David Riebenbauer <davrieb@sbox.tugraz.at>

#include "util/string.h"
#include "mm/kmalloc.h"
#include "assert.h"


//----------------------------------------------------------------------
size_t strlen(const char *str)
{
  const char *pos = str;

  while (*pos)
  {
    ++pos;
  }

  return (pos - str);
}


//----------------------------------------------------------------------
void *memcpy(void *dest, const void *src, size_t length)
{
  uint8 *dest8 = (uint8*)dest;
  const uint8 *src8 = (const uint8*)src;

#ifdef STRING_SAVE
  if (((src8 < (dest8 + length)) && (src8 > dest8))
      || ((dest8 < (src8 + length)) && (dest8 > src8)))
  {
    return dest;
    // error because strings overlap with is not allowed for memcpy
  }
#endif

  if (length == 0 || src == dest)
  {
    return dest;
  }

  while (length--)
  {
      *dest8++ = *src8++;
  }

  return dest;
}


//----------------------------------------------------------------------
void *memmove(void *dest, const void *src, size_t length)
{
  uint8* dest8 = (uint8*)dest;
  const uint8* src8 = (const uint8*) src;

  if (length == 0 || src == dest)
  {
    return dest;
  }

  if (src < dest)
  {
    // if src is before dest we can do a forward copy
    while (length--)
    {
      *dest8++ = *src8++;
    }
  }
  else
  {
    // if src is _not_ before dest we have to do a backward copy
    src8 += length;
    dest8 += length;

    while (length--)
    {
      *dest8-- = *src8--;
    }
  }

  return dest;
}


//----------------------------------------------------------------------
void *memccpy(void *dest, const void *src, uint8 c, size_t length)
{
  uint8 *dest8 = (uint8*)dest;
  const uint8 *src8 = (const uint8*)src;

  if (length == 0)
  {
    return (void*)0;
  }

  while (length--)
  {
    if ((*dest8++ = *src8++) == c)
    {
      return (void*)dest8;
    }
  }

  return (void *)0;
}


//----------------------------------------------------------------------
void *memset(void *block, uint8 c, size_t size)
{
  uint8 *block8 = (uint8*)block;

  if (size)
  {
    while (size--)
    {
      *block8++ = c;
    }
  }

  return block;
}


//----------------------------------------------------------------------
char *strcpy(char *dest, const char* src)
{
  char *start = dest;

  for(; (*dest = *src); ++src, ++dest)
    ;

  return start;
}


//----------------------------------------------------------------------
char *strncpy(char *dest, const char* src, size_t size)
{
  char *start = dest;
  int8 fill = 0;

  while (size--)
  {
    if (fill)
    {
      *dest = 0;
    }
    else if ((*dest = *src) == 0)
    {
      fill = 1;
    }

    src++;
    dest++;
  }

  return start;
}


//----------------------------------------------------------------------
size_t strlcpy(char* dest, const char* src, size_t size)
{
    const char* src_start = src;
    size_t n = size;

    if(n > 1)
    {
      --n;

      while (n--)
      {
        if ((*dest++ = *src++) == 0)
        {
          break;
        }
      }
    }

    // terminate dest, if it was not done already
    if (n == 0 && *(dest - 1))
    {
      *dest = '\0';
    }

    while (*src)
    {
      ++src;
    }

    return (src - src_start);
}


//----------------------------------------------------------------------
char *strdup(const char *src)
{
  size_t size = strlen(src) + 1;
  char *dest = 0;

  if ((dest = (char*)kmalloc((size) * sizeof(char))) == (char*)0)
  {
    return (char*)0;
  }

  return (char*)memcpy(dest, src, size);
}


//----------------------------------------------------------------------
char *strcat(char *dest, const char*append)
{
  char *start = dest + strlen(dest);
  strcpy(start, append);
  return dest;
}


//----------------------------------------------------------------------
char *strncat(char *dest, const char*append, size_t size)
{
  char* save = dest;

  if (size == 0)
  {
    return save;
  }

  while (*dest)
  {
    ++dest;
  }

  while (size--)
  {
    if ((*dest = *append++) == '\0')
    {
      break;
    }
    ++dest;
  }

  *dest= '\0';
  return save;
}


//----------------------------------------------------------------------
size_t strlcat(char *dest, const char*append, size_t size)
{
  size_t count = size;
  const char*append_start = append;
  size_t done = 0;

  while (count != 0 && *dest != '\0')
  {
    --count;
    ++dest;
  }
  done = size - count;

  if (count == 0)
  {
    return done + strlen(append);
  }

  while (count--)
  {
    if ((*dest++ = *append) == '\0')
    {
      break;
    }
    ++append;
  }

  return done + (append - append_start) - 1;
}


//----------------------------------------------------------------------
void bcopy(void *src, void* dest, size_t length)
{
  uint8* dest8 = (uint8*)dest;
  const uint8* src8 = (const uint8*) src;

  if (length == 0 || src == dest)
  {
    return;
  }

  if (src < dest)
  {
    // if src is before dest we can do a forward copy
    while (length--)
    {
      *dest8++ = *src8++;
    }
  }
  else
  {
    // if src is _not_ before dest we can do a forward copy
    src8 += length;
    dest8 += length;

    while (length--)
    {
      *dest8-- = *src8--;
    }
  }

}


//----------------------------------------------------------------------
void bzero(void *block, size_t size)
{
  uint8 *block8 = (uint8*)block;

  if (size)
  {
    while (size--)
    {
      *block8++ = 0;
    }
  }

}


//----------------------------------------------------------------------
int32 memcmp(const void *region1, const void *region2, size_t size)
{
  const uint8* b1 = region1;
  const uint8* b2 = region2;

  if (size == 0)
  {
    return 0;
  }

  while (size--)
  {
    if (*b1++ != *b2++)
    {
      return (*--b1 - *--b2);
    }
  }

  return 0;
}


//----------------------------------------------------------------------
int32 strcmp(const char *str1, const char *str2)
{
  assert(str1);
  assert(str2);

  if (str1 == str2)
  {
    return 0;
  }

  while ((*str1) && (*str2))
  {
    if (*str1 != *str2)
    {
      break;
    }
    ++str1;
    ++str2;
  }

  return (*(uint8 *)str1 - *(uint8 *)str2);
}


//----------------------------------------------------------------------
int32 strncmp(const char *str1, const char *str2, size_t n)
{
  while (n-- && (*str1) && (*str2))
  {
    if (*str1 != *str2)
    {
      break;
    }
    ++str1;
    ++str2;
  }

  return (*(uint8 *)str1 - *(uint8 *)str2);
}


//----------------------------------------------------------------------
int32 bcmp(const void *region1, const void *region2, size_t size)
{
  const uint8* b1 = region1;
  const uint8* b2 = region2;

  if (size == 0)
  {
    return 0;
  }

  while (size--)
  {
    if (*b1++ != *b2++)
    {
      return (*--b1 - *--b2);
    }
  }

  return 0;
}


//----------------------------------------------------------------------
void *memchr(const void *block, uint8 c, size_t size)
{
  const uint8 *b = (const uint8*)block;

  while (size--)
  {
    if (*b == c)
    {
      return (void *)b;
    }
    ++b;
  }

  return (void *)0;
}


//----------------------------------------------------------------------
void *memrchr(const void *block, uint8 c, size_t size)
{
  const uint8 *b = (const uint8*)(block + size - 1);

  while (size--)
  {
    if (*b == c)
    {
      return (void *)b;
    }
    --b;
  }

  return (void *)0;
}


//----------------------------------------------------------------------
char *strchr(const char* str, char c)
{
  do
  {
    if (*str == c)
    {
      return (char *)str;
    }
  } while (*++str);

  return (char *)0;
}


//----------------------------------------------------------------------
char *strrchr(const char* str, char c)
{
  uint32 len = strlen(str);
  const char *pos = str + len; // goes to '\0'

  do
  {
    if (*--pos == c)
    {
      return (char *)pos;
    }
  }
  while (--len);

  return (char *)0;
}




