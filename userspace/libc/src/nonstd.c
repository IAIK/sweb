#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int createprocess(const char* path, int sleep, size_t cpu)
{
  return __syscall(sc_createprocess, (long) path, sleep, cpu, 0x00, 0x00);
}

extern int main();

void _start()
{
  exit(main());
}
