#ifndef __PAGING_DEFINITIONS_H__
#define __PAGING_DEFINITIONS_H__

#include "types.h"


#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096
#define PAGE_INDEX_OFFSET_BITS 12

#define PHYSICAL_OFFSET     0xC0000000


// Constants for page fault handling
#define ERROR_MASK            0x00000007
#define PAGE_FAULT_USER       0x00000004
#define PAGE_FAULT_WRITEABLE  0x00000002
#define PAGE_FAULT_PRESENT    0x00000001


struct PageDirPageTableEntry
{
  uint32 present                   :1;
  uint32 writeable                 :1;
  uint32 user_access               :1;
  uint32 write_through             :1;
  uint32 cache_disabled            :1;
  uint32 accessed                  :1;
  uint32 ignored_5                 :1;
  uint32 size                      :1;
  uint32 ignored_4                 :1;
  uint32 ignored_3                 :1;
  uint32 ignored_2                 :1;
  uint32 ignored_1                 :1;
  uint32 page_table_ppn            :20;
} __attribute__((__packed__));

struct PageDirPageEntry
{
  uint32 present                   :1;
  uint32 writeable                 :1;
  uint32 user_access               :1;
  uint32 write_through             :1;
  uint32 cache_disabled            :1;
  uint32 accessed                  :1;
  uint32 dirty                     :1;
  uint32 size                      :1;
  uint32 global_page               :1;
  uint32 ignored_3                 :1;
  uint32 ignored_2                 :1;
  uint32 ignored_1                 :1;

  uint32 pat                       :1;
  uint32 reserved                  :9;
  uint32 page_ppn                  :10;
} __attribute__((__packed__));

typedef union
{
  struct PageDirPageTableEntry pt;
  struct PageDirPageEntry page;
} __attribute__((__packed__)) PageDirEntry;

typedef struct
{
  uint32 present                   :1;
  uint32 writeable                 :1;
  uint32 user_access               :1;
  uint32 write_through             :1;
  uint32 cache_disabled            :1;
  uint32 accessed                  :1;
  uint32 dirty                     :1;
  uint32 pat                       :1;
  uint32 global_page               :1;
  uint32 ignored_3                 :1;
  uint32 ignored_2                 :1;
  uint32 ignored_1                 :1;
  uint32 page_ppn                  :20;
} __attribute__((__packed__)) PageTableEntry;

#endif
