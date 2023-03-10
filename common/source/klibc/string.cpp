#include "string.h"
#include "stdint.h"
#include "assert.h"

extern "C" bool currentThreadIsStackCanaryOK();

extern "C" char *strcpy(char *dest, const char* src)
{
    assert(!"don't use strcpy");

    char *start = dest;

    for (; (*dest = *src); ++src, ++dest)
        ;

    return start;
}

extern "C" char *strncpy(char *dest, const char* src, size_t size)
{
    char *start = dest;
    int8_t fill = 0;

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

    assert(currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

    return start;
}

extern "C" char *strcat(char *dest, const char*append)
{
    char *start = dest + strlen(dest);
    strcpy(start, append);
    return dest;
}

extern "C" char *strncat(char *dest, const char*append, size_t size)
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

    *dest = '\0';
    return save;
}

extern "C" size_t strlen(const char *str)
{
    const char *pos = str;

    while (*pos)
    {
        ++pos;
    }

    return (pos - str);
}

extern "C" int strcmp(const char *str1, const char *str2)
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

    return (*(uint8_t *) str1 - *(uint8_t *) str2);
}

extern "C" int strncmp(const char *str1, const char *str2, size_t n)
{
    while (n && (*str1) && (*str2))
    {
        if (*str1 != *str2)
        {
            break;
        }
        ++str1;
        ++str2;
        --n;
    }
    if (n == 0)
        return 0;
    else
        return (*(uint8_t *) str1 - *(uint8_t *) str2);
}

extern "C" char *strchr(const char* str, char c)
{
    do
    {
        if (*str == c)
        {
            return (char *) str;
        }
    } while (*++str);

    return (char *) nullptr;
}

extern "C" char *strrchr(const char* str, char c)
{
    size_t len = strlen(str);
    const char *pos = str + len; // goes to '\0'

    do
    {
        if (*--pos == c)
        {
            return (char *) pos;
        }
    } while (--len);

    return (char *) nullptr;
}

extern "C" char* strtok(char* str, const char* delimiters)
{
  static char* str_to_tok = nullptr;
  if (str != nullptr)
    str_to_tok = str;

  // no delimiters, so just return the rest-string
  if (delimiters == nullptr)
    return str_to_tok;

  if (str_to_tok == nullptr)
    return nullptr;

  // determine token start and end
  size_t tok_start = 0;
  size_t tok_end = -1;

  // find first char which is not one of the delimiters
  size_t str_pos = 0;
  for (str_pos = 0; str_to_tok[str_pos] != '\0'; str_pos++)
  {
    uint8_t char_is_delimiter = 0;

    size_t del_pos = 0;
    for (del_pos = 0; delimiters[del_pos] != '\0'; del_pos++)
    {
      if (str_to_tok[str_pos] == delimiters[del_pos])
      {
        char_is_delimiter = 1;
        break;
      }
    }

    if (char_is_delimiter == 0)
    {
      // this is the start char of the token
      tok_start = str_pos;
      break;
    }
  }

  // find next delimiter in the string
  for (str_pos = tok_start; str_to_tok[str_pos] != '\0'; str_pos++)
  {
    size_t del_pos = 0;
    for (; delimiters[del_pos] != '\0'; del_pos++)
    {
      if (str_to_tok[str_pos] == delimiters[del_pos])
      {
        // delimiter found!
        tok_end = str_pos;
        break;
      }
    }

    if (tok_end != -1U)
      break;
  }

  // create and return token:
  char* token = str_to_tok + tok_start;

  // update string
  if (tok_end == -1U)
  {
    // finished, no next token
    str_to_tok = nullptr;
  }
  else
  {
    str_to_tok[tok_end] = '\0';
    str_to_tok += tok_end + 1;
  }

  return token;
}

extern "C" void *memchr(const void *block, char c, size_t size)
{
    const char *b = (const char*) block;

    while (size--)
    {
        if (*b == c)
        {
            return (void *) b;
        }
        ++b;
    }

    return (void *) nullptr;
}

extern "C" int memcmp(const void *region1, const void *region2, size_t size)
{
    const char* b1 = (const char*)region1;
    const char* b2 = (const char*)region2;

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

extern "C" void *memset(void *block, char c, size_t size)
{
    if (size)
    {
        size_t i;
        size_t* d = (size_t*) block;
        size_t large_c = c;
        for (i = 0; i < sizeof(size_t); i++)
        {
            large_c = (large_c << 8) | c;
        }
        size_t num_large_copies = size / sizeof(size_t);
        size_t num_rest_copies = size % sizeof(size_t);
        for (i = 0; i < num_large_copies; ++i)
        {
            *d++ = large_c;
        }
        uint8_t* d8 = (uint8_t*) d;
        for (i = 0; i < num_rest_copies; ++i)
        {
            *d8++ = c;
        }
    }

    assert(currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

    return block;
}

extern "C" void *memcpy(void *dest, const void *src, size_t length)
{
    size_t* s = (size_t*) src;
    size_t* d = (size_t*) dest;
    size_t num_large_copies = length / sizeof(size_t);
    size_t num_rest_copies = length % sizeof(size_t);
    for (size_t i = 0; i < num_large_copies; ++i)
    {
        *d++ = *s++;
    }
    uint8_t* s8 = (uint8_t*) s;
    uint8_t* d8 = (uint8_t*) d;
    for (size_t i = 0; i < num_rest_copies; ++i)
    {
        *d8++ = *s8++;
    }

    assert(currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

    return dest;
}

extern "C" void *memmove(void *dest, const void *src, size_t length)
{
    uint8_t* dest8 = (uint8_t*) dest;
    const uint8_t* src8 = (const uint8_t*) src;

    if (length == 0 || src == dest)
    {
        return dest;
    }

    if (src > dest)
    {
        // if src is _not_ before dest we can do a forward copy
        while (length--)
        {
            *dest8++ = *src8++;
        }
    }
    else
    {
        // if src is before dest we have to do a backward copy
        src8 += length - 1;
        dest8 += length - 1;

        while (length--)
        {
            *dest8-- = *src8--;
        }
    }

    assert(currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");

    return dest;
}
