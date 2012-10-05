
#include "math.h"

/*
int32 pow(int32 base, uint32 exponent)
{
  int32 result = 1;

  for(uint32 i = 0; i < exponent; i++)
  {
    result *= base;
  }

  return result;
}
*/

uint32 pow(uint32 base, uint32 exponent)
{
  uint32 result = 1;

  uint32 i = 0;
  for(i = 0; i < exponent; i++)
  {
    result *= base;
  }

  return result;
}
