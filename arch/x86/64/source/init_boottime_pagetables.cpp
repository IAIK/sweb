#include "kprintf.h"
#include "multiboot.h"
#include "offsets.h"
#include "paging-definitions.h"

#include "ArchCommon.h"
#include "ArchMemory.h"

#include "types.h"

extern void* kernel_end_address;

extern "C" void initialisePaging()
{
  extern void* kernel_start_address;
  extern void* kernel_end_address;
  extern size_t text_start_address;
  extern size_t ro_data_end_address;
  extern size_t apstartup_text_begin;
  extern size_t apstartup_text_end;

  PageMapLevel4Entry *pml4 = (PageMapLevel4Entry*)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);
  PageDirPointerTableEntry *pdpt1 = (PageDirPointerTableEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_directory_pointer_table);
  PageDirPointerTableEntry *pdpt2 = pdpt1 + PAGE_DIR_POINTER_TABLE_ENTRIES;
  PageDirEntry *pd1 = (PageDirEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_directory);
  PageDirEntry *pd2 = pd1 + PAGE_DIR_ENTRIES;

  PageTableEntry *pt =  (PageTableEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_table);

  VAddr k_start{(size_t)&kernel_start_address};
  VAddr k_end{(size_t)&kernel_end_address};

  // Note: the only valid address ranges are currently:
  //         * 0000 0000 0 to * 7FFF FFFF F
  //                       binary: 011 111 111 means pml4i <= 255
  //         * 8000 0000 0 to * FFFF FFFF F
  //                       binary: 100 000 000 means pml4i >= 256

  // map the first and the last PM4L entry and one for the identity mapping
  // boot time ident mapping
  pml4[0].page_ppn = (uint64)pdpt1 / PAGE_SIZE;
  pml4[0].writeable = 1;
  pml4[0].present = 1;

  // ident mapping
  pml4[480].page_ppn = (uint64)pdpt1 / PAGE_SIZE;
  pml4[480].writeable = 1;
  pml4[480].present = 1;

  // kernel
  pml4[511].page_ppn = (uint64)pdpt2 / PAGE_SIZE;
  pml4[511].writeable = 1;
  pml4[511].present = 1;

  // ident mapping 0x0                <-> 0x0 --> pml4i = 0, pdpti = 0
  // ident mapping 0x* F000 0000 0000 <-> 0x0 --> pml4i = 480, pdpti = 0
  // ident mapping 0x* FFFF 8000 0000 <-> 0x0 --> pml4i = 511, pdpti = 510

  // pdpt for ident mapping
  pdpt1[0].pd.page_ppn = (uint64) pd1 / PAGE_SIZE;
  pdpt1[0].pd.writeable = 1;
  pdpt1[0].pd.present = 1;

  // 1 GiB for the kernel
  pdpt2[510].pd.page_ppn = (uint64) pd2 / PAGE_SIZE;
  pdpt2[510].pd.writeable = 1;
  pdpt2[510].pd.present = 1;

  // set up ident mapping
  for (size_t i = 0; i < PAGE_DIR_ENTRIES; ++i)
  {
    pd2[i].page.present = 0;

    pd1[i].page.page_ppn = i;
    pd1[i].page.size = 1;
    pd1[i].page.writeable = 1;
    pd1[i].page.cache_disabled = 0;
    pd1[i].page.present = 1;
  }

  // Map 8 page directory entries for kernel (8*512*4kb = max 16mb)
  for (size_t pdi = k_start.pdi; pdi <= k_end.pdi; ++pdi)
  {
    assert(pdi < PAGE_DIR_ENTRIES);
    size_t pdi_offset = pdi - k_start.pdi;

    pd2[pdi].pt.writeable = 1;
    pd2[pdi].pt.present = 1;
    pd2[pdi].pt.page_ppn = ((pointer)&pt[PAGE_TABLE_ENTRIES*pdi_offset])/PAGE_SIZE;
  }


    // AP startup pages need to be writeable to fill in the GDT, ...
  size_t ap_text_start = ((size_t)&apstartup_text_begin/PAGE_SIZE)*PAGE_SIZE;
  size_t ap_text_end = (size_t)&apstartup_text_end;

  // Map the kernel page tables (first 640kib = 184 pages are unused)
  assert(k_start.addr + PAGE_SIZE < k_end.addr);
  for(VAddr a{k_start}; a.addr < k_end.addr; a.addr += PAGE_SIZE)
  {
    size_t pti = (a.pdi - k_start.pdi)*PAGE_TABLE_ENTRIES + a.pti;
    assert(pti < sizeof(kernel_page_table)/sizeof(kernel_page_table[0]));

    pt[pti].page_ppn = (uint64)VIRTUAL_TO_PHYSICAL_BOOT(a.addr)/PAGE_SIZE;
    pt[pti].writeable =
        (((a.addr >= (pointer)&text_start_address) &&
          (a.addr < (pointer)&ro_data_end_address)) &&
         !((a.addr >= ap_text_start) &&
          (a.addr < ap_text_end))
             ? 0
             : 1);
    pt[pti].present = 1;
  }

  // Map framebuffer
  VAddr framebuffer_start{ArchCommon::getFBPtr()};
  VAddr framebuffer_end{ArchCommon::getFBPtr() + ArchCommon::getFBSize()};
  for(VAddr a{framebuffer_start}; a.addr < framebuffer_end.addr; a.addr += PAGE_SIZE)
  {
    size_t pti = (a.pdi - framebuffer_start.pdi)*PAGE_TABLE_ENTRIES + a.pti;

    pt[pti].page_ppn = (uint64)VIRTUAL_TO_PHYSICAL_BOOT(a.addr)/PAGE_SIZE;
    pt[pti].writeable = 1;
    pt[pti].write_through = 1;
    pt[pti].present = 1;
  }

  if (ArchCommon::haveVESAConsole(0))
  {
    for (size_t i = 0; i < 8; ++i) // map the 16 MiB (8 pages) framebuffer
    {
      pd2[504+i].page.present = 1;
      pd2[504+i].page.writeable = 1;
      pd2[504+i].page.size = 1;
      pd2[504+i].page.cache_disabled = 1;
      pd2[504+i].page.write_through = 1;
      pd2[504+i].page.page_ppn = (ArchCommon::getVESAConsoleLFBPtr(0) / (PAGE_SIZE * PAGE_TABLE_ENTRIES))+i;
    }
  }
}

extern "C" void removeBootTimeIdentMapping()
{
  uint64* pml4 = (uint64*)&kernel_page_map_level_4[0];
  pml4[0] = 0;
  pml4[1] = 0;
}
