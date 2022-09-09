#include "ArchCpuLocalStorage.h"
#include "debug.h"
#include <cstring>

extern char cls_start;
extern char cls_end;
extern char tbss_start;
extern char tbss_end;
extern char tdata_start;
extern char tdata_end;


void* CpuLocalStorage::getClsBase()
{
    void* cls = 0;
    asm("MRS %[cls], TPIDR_EL1\n"
        :[cls]"=g"(cls));
    return cls;
}

bool CpuLocalStorage::ClsInitialized()
{
    return getClsBase();
}

size_t CpuLocalStorage::getClsSize()
{
    return &cls_end - &cls_start;
}

char* CpuLocalStorage::allocCls()
{
  debug(A_MULTICORE, "Allocating CPU local storage\n");

  size_t cls_size = getClsSize();
  size_t tbss_size = &tbss_end - &tbss_start;
  size_t tdata_size = &tdata_end - &tdata_start;
  debug(A_MULTICORE, "cls_base: [%p, %p), size: %zx\n", &cls_start, &cls_end, cls_size);
  debug(A_MULTICORE, "tbss: [%p, %p), size: %zx\n", &tbss_start, &tbss_end, tbss_size);
  debug(A_MULTICORE, "tdata: [%p, %p), size: %zx\n", &tdata_start, &tdata_end, tdata_size);

  char* cls_base = new char[cls_size + sizeof(void*)]{};
  debug(A_MULTICORE, "Allocated new cls_base at [%p, %p)\n", cls_base, cls_base + cls_size + sizeof(void*));

  debug(A_MULTICORE, "Initializing tdata at [%p, %p) and tbss at [%p, %p)\n",
        cls_base + (&tdata_start - &cls_start), cls_base + (&tdata_start - &cls_start) + tdata_size,
        cls_base + (&tbss_start - &cls_start), cls_base + (&tbss_start - &cls_start) + tbss_size);
  memcpy(cls_base + (&tdata_start - &cls_start), &tdata_start, tdata_size);

  return cls_base;
}

void CpuLocalStorage::setCls(char* cls)
{
    debug(A_MULTICORE, "Set CLS: %p\n", cls);
    asm("MSR TPIDR_EL1, %[cls]\n"
        ::[cls]"g"(cls));
}
