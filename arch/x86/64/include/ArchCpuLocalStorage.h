#pragma once

#include <cstddef>

#define cpu_local thread_local
#define __cpu __thread

namespace CpuLocalStorage
{
    void initCpuLocalStorage();
    size_t getClsSize();

    char* allocCls();
    void setCls(char* cls);
    bool ClsInitialized();
};
