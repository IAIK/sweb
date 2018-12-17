#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

int getcpu(size_t *cpu, size_t *node, void *tcache)
{
  return __syscall(sc_getcpu, (size_t)cpu, (size_t)node, (size_t)tcache, 0x00, 0x00);
}

extern int main();

void _start()
{
  exit(main());
}
