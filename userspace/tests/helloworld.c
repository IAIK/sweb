#include "stdio.h"
#include "nonstd.h"
#include "assert.h"

int main(int argc, char *argv[])
{
        size_t cpu = 0;
        assert(getcpu(&cpu, NULL, NULL) == 0);
        printf("Hello from CPU %zu\n", cpu);
}
