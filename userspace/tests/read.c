#include "../../common/include/kernel/syscall-definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
  *(size_t*)0x800000000000 = 1;
  return 0;
}
