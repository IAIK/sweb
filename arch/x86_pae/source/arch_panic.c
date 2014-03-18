/**
 * @file arch_panic.c
 *
 */
 
#include "arch_panic.h"

void arch_panic(uint8 *mesg)
{
  uint8 *fb = (uint8*)0xC00B8000;
  uint32 i=0;
  while (mesg && *mesg)
  {
    fb[i++] = *mesg++;
    fb[i++] = 0x9f;
  }
  for (;;);
}
