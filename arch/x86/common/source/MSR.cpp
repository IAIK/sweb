#include "MSR.h"
#include "debug.h"
#include <cinttypes>


void getMSR(uint32 msr, uint32 *lo, uint32 *hi)
{
        asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void setMSR(uint32 msr, uint32 lo, uint32 hi)
{
        // if(A_MULTICORE & OUTPUT_ADVANCED)
        // {
        //         debug(A_MULTICORE, "Set MSR %x, value: %" PRIx64 "\n", msr, ((uint64)hi << 32)| (uint64)lo);
        // }
        asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
