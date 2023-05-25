#pragma once

#include <cstddef>

#define cpu_local
#define __cpu

namespace CpuLocalStorage
{
    size_t getClsSize();

    char* allocCls();
    void setCls(char* cls);
    bool ClsInitialized();
    void* getClsBase();
};
