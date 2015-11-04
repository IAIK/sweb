#include "stdlib.h"

void __attribute__((naked)) __syscall()
{
  asm("push {r4,r5}\n"
      "ldr r4, [fp, #-12]\n"
      "ldr r5, [fp, #-8]\n"
      "svc 0\n"
      "pop {r4,r5}\n"
      "bx lr");
}

void abort()
{
  exit(-127);
}
