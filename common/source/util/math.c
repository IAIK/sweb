
#include "kmath.h"

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
