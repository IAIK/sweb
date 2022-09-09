#pragma once

#define cpu_local
#define __cpu


namespace CpuLocalStorage
{
    // size_t getCLSSize();

    // char* allocCLS();
    // void setCLS(char* cls);
    bool ClsInitialized();
    // void* getClsBase();
};
