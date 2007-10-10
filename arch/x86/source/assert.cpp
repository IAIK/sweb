/**
 * @file assert.cpp
 *
 */
 
#include "assert.h"
#include "kprintf.h"
#include "panic.h"


void pre_new_sweb_assert(uint32 condition, uint32 line, char* file)
{
  char *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line: ";
  if (!condition)
  {
    uint8 * fb = (uint8*)0xC00B8000;
    uint32 s=0;
    uint32 i=0;
    for (s=0; s<40; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;      
    }

    while (file && *file)
    {
      fb[i++] = *file++;
      fb[i++] = 0x9f;
    }

    for (s=40; s<50; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;
    }

    i+=6;
    while (line>0)
    {
      fb[i++] = (uint8) ( 0x30 + (line%10) );
      fb[i] = 0x9f; 
      i-=2;
      i--;
      line /= 10;
    }

    for (;;);
  }
}


void sweb_assert(const char *condition, uint32 line, const char* file)
{
  kprintf_nosleep("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kpanict((uint8*) "Halting System\n");
}
