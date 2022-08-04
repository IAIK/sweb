#include "stdio.h"
#include "nonstd.h"
#include "assert.h"


int main(int argc, char *argv[])
{
        printf("Starting test loop\n");
        while(1)
        {
            size_t cpu = 0;
            assert(getcpu(&cpu, NULL, NULL) == 0);
            printf("Hello from CPU %zu\n", cpu);
        }
}
