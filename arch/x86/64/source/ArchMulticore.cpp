#include "ArchMulticore.h"
#include "debug.h"
#include "APIC.h"

void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
        asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
        debug(A_MULTICORE, "Set MSR %x, value: %zx\n", msr, ((size_t)hi << 32) | (size_t)lo);
        asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102

uint64 getGSBase()
{
        uint64 gs_base;
        cpuGetMSR(MSR_GS_BASE, (uint32*)&gs_base, ((uint32*)&gs_base) + 1);
        return gs_base;
}

void setGSBase(uint64 gs_base)
{
        cpuSetMSR(MSR_GS_BASE, gs_base, gs_base >> 32);
}

void setSWAPGSKernelBase(uint64 swapgs_base)
{
        cpuSetMSR(MSR_KERNEL_GS_BASE, swapgs_base, swapgs_base >> 32);
}

void setCLS(CoreLocalStorage* cls)
{
        debug(A_MULTICORE, "Set CLS to %p\n", cls);
        cls->cls_ptr = cls;
        cls->core_id = local_APIC.getID();
        setGSBase((uint64)cls);
        setSWAPGSKernelBase((uint64)cls);
}

CoreLocalStorage* getCLS()
{
        CoreLocalStorage* cls_ptr;
        assert(getGSBase() != 0); // debug only
        __asm__ __volatile__("movq %%gs:0, %%rax\n"
                             "movq %%rax, %[cls_ptr]\n"
                             : [cls_ptr]"=m"(cls_ptr));
        return cls_ptr;
}

CoreLocalStorage* initCLS()
{
        CoreLocalStorage* cls = new CoreLocalStorage{};
        setCLS(cls);
        return getCLS();
}


size_t getCoreID()
{
        return getCLS()->core_id;
}
