#include "MSR.h"
#include "debug.h"


void getMSR(uint32 msr, uint32 *lo, uint32 *hi)
{
        asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void setMSR(uint32 msr, uint32 lo, uint32 hi)
{
        if(A_MULTICORE & OUTPUT_ADVANCED)
        {
                debug(A_MULTICORE, "Set MSR %x, value: %zx\n", msr, ((size_t)hi << 32) | (size_t)lo);
        }
        asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
