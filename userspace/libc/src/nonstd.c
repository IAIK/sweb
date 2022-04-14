#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

typedef void (*func_ptr)();

extern int main();

// Symbols automatically created by linker
// https://maskray.me/blog/2021-11-07-init-ctors-init-array
extern func_ptr __preinit_array_start;
extern func_ptr __preinit_array_end;

extern func_ptr __init_array_start;
extern func_ptr __init_array_end;


int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

int getcpu(size_t *cpu, size_t *node, void *tcache)
{
  return __syscall(sc_getcpu, (size_t)cpu, (size_t)node, (size_t)tcache, 0x00, 0x00);
}

ssize_t getdents(int fd, char* buffer, size_t buffer_size)
{
  return __syscall(sc_getdents, (size_t)fd, (size_t)buffer, (size_t)buffer_size, 0, 0);
}

void _preinit()
{
    func_ptr* it = &__preinit_array_start;
	while(it != &__preinit_array_end)
        (*it++)();
}

// Call constructors
void _init()
{
    func_ptr* it = &__init_array_start;
	while(it != &__init_array_end)
        (*it++)();
}

void _start()
{
  _preinit();
  _init();
  exit(main());
}
