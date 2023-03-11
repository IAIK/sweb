#include "ArchCpuLocalStorage.h"

#include "SegmentUtils.h"

#include <cstring>

#include "debug.h"

extern char cls_start;
extern char cls_end;
extern char tbss_start;
extern char tbss_end;
extern char tdata_start;
extern char tdata_end;

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

void CpuLocalStorage::setCls(GDT& gdt, char* cls)
{
  debug(A_MULTICORE, "Set CLS: %p\n", cls);
  void** gs_base = (void**)(cls + getClsSize());
  debug(A_MULTICORE, "Init CLS pointer at %%gs:0 = %p\n", gs_base);
  *gs_base = gs_base;

  // %gs base needs to point to end of CLS, not the start. %gs:0 = pointer to %gs base
  setGSBase(gdt, (size_t)gs_base);
  setFSBase(gdt, (size_t)gs_base);

  debug(A_MULTICORE, "FS base: %p\n", (void*)getFSBase(gdt));
  debug(A_MULTICORE, "GS base: %p\n", (void*)getGSBase(gdt));
}

bool CpuLocalStorage::ClsInitialized()
{
  uint32_t gs_val = 0;
  asm("mov %%gs, %[gs]\n"
      :[gs]"=g"(gs_val));

  return gs_val == KERNEL_GS;
}

void* CpuLocalStorage::getClsBase()
{
  void *gs_base = 0;
  asm("movl %%gs:0, %[gs_base]\n" : [gs_base] "=r"(gs_base));
  return gs_base;
}
