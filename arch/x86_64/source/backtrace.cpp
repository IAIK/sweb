//-------------------------------------------------------------------------------------*/
#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "mm/KernelMemoryManager.h" // for use of "kernel_end_address"
#include "ustl/umap.h"
#include "ArchCommon.h"

//-------------------------------------------------------------------------------------*/

uint64 parse_leb128(uint8*& data)
{
  uint64 value = 0;
  uint64 i = 0;
  do
  {
    value |= (*data & 0x7F) << i;
    data++;
    i += 7;
  } while (*data & 0x80 != 0);
  return value;
}

//
//extern uint64 debug_info_start;
//extern uint64 debug_info_end;
//
//extern uint64 debug_aranges_start;
//extern uint64 debug_aranges_end;
//
//extern uint64 debug_line_start;
//extern uint64 debug_line_end;

void parse_dwarf()
{
//  kprintfd("aranges: %x to %x\n", &debug_aranges_start, &debug_aranges_end);
//  uint8* aranges = (uint8*)&debug_aranges_start;
//  uint8* aranges_end = (uint8*)&debug_aranges_end;
//  while (aranges < aranges_end)
//  {
//    uint64 index = parse_leb128(aranges);
//    kprintfd("aranges: %x, index: %x\n", aranges, index);
//    uint64 version = parse_leb128(aranges);
//    kprintfd("aranges: %x, version: %x\n", aranges, version);
//    uint64 offset = parse_leb128(aranges);
//    kprintfd("aranges: %x, offset: %x\n", aranges, offset);
//    uint64 addrsize = parse_leb128(aranges);
//    kprintfd("aranges: %x, addrsize: %x\n", aranges, addrsize);
//    uint64 segsize = parse_leb128(aranges);
//    kprintfd("aranges: %x, segsize: %x\n", aranges, segsize);
//  }
}

struct StabEntry
{
    uint32 n_strx;
    uint8 n_type;
    uint8 n_other;
    uint16 n_desc;
    uint32 n_value;
} __attribute__((packed));
int backtrace(pointer *call_stack, int size, Thread *thread,
    bool use_stored_registers)
{
  return 0;
}
pointer get_function_name(pointer address, char function_name[])
{
  return 0;
}
void parse_symtab(StabEntry *stab_start, StabEntry *stab_end, const char *stab_str)
{
}
