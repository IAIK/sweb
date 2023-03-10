#include "kstring.h"
#include "kmalloc.h"
#include "assert.h"
#include "ArchMemory.h"
#include "Thread.h"

void *memccpy(void *dest, const void *src, uint8 c, size_t length)
{
  uint8 *dest8 = (uint8*) dest;
  const uint8 *src8 = (const uint8*) src;

  if (length == 0)
  {
    return (void*) nullptr;
  }

  while (length--)
  {
    if ((*dest8++ = *src8++) == c)
    {
      return (void*) dest8;
    }
  }

  assert(Thread::currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

  return (void *) nullptr;
}

extern "C" char *strdup(const char *src)
{
  size_t size = strlen(src) + 1;
  char *dest = nullptr;

  if ((dest = (char*) kmalloc((size) * sizeof(char))) == (char*) nullptr)
  {
    return (char*) nullptr;
  }

  return (char*) memcpy(dest, src, size);
}

extern "C" size_t strlcat(char *dest, const char*append, size_t size)
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

extern "C" int32 bcmp(const void *region1, const void *region2, size_t size)
{
  const uint8* b1 = (const uint8*)region1;
  const uint8* b2 = (const uint8*)region2;

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

extern "C" void *memnotchr(const void *block, uint8 c, size_t size)
{
  const uint8 *b = (const uint8*) block;

  while (size--)
  {
    if (*b != c)
    {
      return (void *) b;
    }
    ++b;
  }

  return (void *) 0;
}

extern "C" void *memrchr(const void *block, uint8 c, size_t size)
{
  const uint8 *b = ((const uint8*) block + size - 1);

  while (size--)
  {
    if (*b == c)
    {
      return (void *) b;
    }
    --b;
  }

  return (void *) nullptr;
}

// converts a single digit into an
extern "C" char numToASCIIChar(uint8 number)
{
  if (number <= 9)
    return 0x30 + number;

  if (number >= 0xa && number <= 0xf)
    return 0x61 + number - 0xa;

  // default value
  return '?';
}

extern "C" char* itoa(int value, char* str, int base)
{
  if (!str)
    return nullptr;

  int div = value;
  int mod;
  unsigned int str_pos = 0;

  while (div >= base)
  {
    mod = div % base;
    div /= base;
    str[str_pos++] = numToASCIIChar(mod);
  }
  str[str_pos++] = numToASCIIChar(div);
  if (value < 0)
    str[str_pos++] = '-';
  str[str_pos] = '\0';

  if (str_pos > 1)
  {
    uint32 str_len = strlen(str);
    uint32 i = 0;
    // switching the string
    for (i = 0; i < str_len / 2; i++)
    {
      char temp = str[str_len - 1 - i];
      str[str_len - 1 - i] = str[i];
      str[i] = temp;
    }
  }

  assert(Thread::currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

  return str;
}

extern "C" uint32 checksum(const uint32* src, uint32 nbytes)
{
  nbytes /= sizeof(uint32);
  uint32 poly = 0xEDB88320;
  int bit = 0, nbits = 32;
  uint32 res = 0xFFFFFFFF;

  for (uint32 i = 0; i < nbytes; ++i)
    for (bit = nbits - 1; bit >= 0; --bit)
      if ((res & 1) != ((src[i] >> bit) & 1))
        res = (res >> 1) ^ poly;
      else
        res = (res >> 1) + 7;

  res ^= 0xFFFFFFFF;
  return res;
}
