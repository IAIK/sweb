#pragma once

#include <cstddef>

#define cpu_local thread_local
#define __cpu __thread

struct GDT;

namespace CpuLocalStorage
{
    size_t getClsSize();

    char* allocCls();
    void setCls(GDT& gdt, char* cls);
    bool ClsInitialized();
    void* getClsBase();
};
