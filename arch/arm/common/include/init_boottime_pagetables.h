/**
 * @file init_boottime_pagetables.h
 *
 */

#ifndef INIT_BOOTTIME_PAGETABLES_H_
#define INIT_BOOTTIME_PAGETABLES_H_

extern page_directory_entry kernel_page_directory_start[];
extern void* kernel_end_address;
extern void* interrupt_vector_table;
extern char* currentStack;

extern "C" void initialiseBootTimePaging();
extern "C" void removeBootTimeIdentMapping();

#define TO_HEX(X,Y) (((((X) >> Y) & 0xF) < 10) ? (((X) >> Y) & 0xF) + '0' : (((X) >> Y) & 0xF) - 0xA + 'A')
#define PRINT_ADDRESS(X) do { \
kprintfd("%c",TO_HEX((uint32)X,28)); \
kprintfd("%c",TO_HEX((uint32)X,24)); \
kprintfd("%c",TO_HEX((uint32)X,20)); \
kprintfd("%c",TO_HEX((uint32)X,16)); \
kprintfd("%c",TO_HEX((uint32)X,12)); \
kprintfd("%c",TO_HEX((uint32)X,8)); \
kprintfd("%c",TO_HEX((uint32)X,4)); \
kprintfd("%c\n",TO_HEX((uint32)X,0)); \
} while (0)

static void mapPage(page_directory_entry *pde_start, uint32 pdi, uint32 ppn_1m)
{
  pde_start[pdi].pde1m.reserved_1 = 0;
  pde_start[pdi].pde1m.permissions = 1;
  pde_start[pdi].pde1m.reserved_2 = 0;
  pde_start[pdi].pde1m.domain = 0;
  pde_start[pdi].pde1m.reserved_3 = 0;
  pde_start[pdi].pde1m.cachable = 0;
  pde_start[pdi].pde1m.bufferable = 0;
  pde_start[pdi].pde1m.size = 2;
  pde_start[pdi].pde1m.page_ppn = ppn_1m;
}


#endif // INIT_BOOTTIME_PAGETABLES_H_
