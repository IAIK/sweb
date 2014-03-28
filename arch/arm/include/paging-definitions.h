/**
 * @file paging-definitions.h
 *
 */

#ifndef __PAGING_DEFINITIONS_H__
#define __PAGING_DEFINITIONS_H__

#include "types.h"


#define PAGE_DIR_ENTRIES 4096
#define PAGE_TABLE_ENTRIES 256
#define PAGE_SIZE 4096
#define PAGE_INDEX_OFFSET_BITS 12

#define PHYSICAL_OFFSET     0xC0000000


// Constants for page fault handling
#define ERROR_MASK            0x00000007
#define PAGE_FAULT_USER       0x00000004
#define PAGE_FAULT_WRITEABLE  0x00000002
#define PAGE_FAULT_PRESENT    0x00000001

struct page_directory_entry_1m_struct
{
  uint32 size                      :2;  // 1:0
  uint32 bufferable                :1;  // 2
  uint32 cachable                  :1;  // 3
  uint32 reserved_3                :1;  // 4
  uint32 domain                    :4;  // 8:5
  uint32 reserved_2                :1;  // 9
  uint32 permissions               :2;  // 11:10
  uint32 reserved_1                :8;  // 19:12
  uint32 base                      :12; // 31:20
} __attribute__((__packed__));

union page_directory_entry_union
{
  struct page_directory_entry_1m_struct pde1m;
} __attribute__((__packed__));

typedef union page_directory_entry_union page_directory_entry;

#endif
