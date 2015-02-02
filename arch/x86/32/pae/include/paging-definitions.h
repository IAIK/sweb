#ifndef __PAGING_DEFINITIONS_H__
#define __PAGING_DEFINITIONS_H__

#include "types.h"


#define PAGE_DIRECTORY_POINTER_TABLE_ENTRIES 4
#define PAGE_DIRECTORY_ENTRIES 512
#define PAGE_TABLE_ENTRIES 512
#define PAGE_SIZE 4096
#define PAGE_INDEX_OFFSET_BITS 12

#define PHYSICAL_OFFSET     0xC0000000


// Constants for page fault handling
#define ERROR_MASK            0x00000007
#define PAGE_FAULT_USER       0x00000004
#define PAGE_FAULT_WRITEABLE  0x00000002
#define PAGE_FAULT_PRESENT    0x00000001


typedef struct
{
  uint64 present                    :1;
  uint64 reserved_1                 :2;  // must be 0
  uint64 write_through              :1;
  uint64 cache_disabled             :1;
  uint64 reserved_2                 :4;  // must be 0
  uint64 ignored_3                  :1;
  uint64 ignored_2                  :1;
  uint64 ignored_1                  :1;
  uint64 page_directory_ppn         :24; // MAXPHYADDR (36) - 12
  uint64 reserved_3                 :28; // must be 0
} __attribute__((__packed__)) PageDirPointerTableEntry;

struct PageDirPageTableEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 ignored_5                 :1;
  uint64 size                      :1;
  uint64 ignored_4                 :1;
  uint64 ignored_3                 :1;
  uint64 ignored_2                 :1;
  uint64 ignored_1                 :1;
  uint64 page_table_ppn            :24; // MAXPHYADDR (36) - 12
  uint64 reserved_2                :27; // must be 0
  uint64 execution_disabled        :1;
} __attribute__((__packed__));

struct PageDirPageEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 dirty                     :1;
  uint64 size                      :1;
  uint64 global_page               :1;
  uint64 ignored_3                 :1;
  uint64 ignored_2                 :1;
  uint64 ignored_1                 :1;

  uint64 pat                       :1;
  uint64 reserved                  :8;
  uint64 page_ppn                  :15; // MAXPHYADDR (36) - 21
  uint64 reserved_2                :27; // must be 0
  uint64 execution_disabled        :1;
} __attribute__((__packed__));

typedef union
{
  struct PageDirPageTableEntry pt;
  struct PageDirPageEntry page;
} __attribute__((__packed__)) PageDirEntry;

typedef struct
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 dirty                     :1;
  uint64 pat                       :1;
  uint64 global_page               :1;
  uint64 ignored_3                 :1;
  uint64 ignored_2                 :1;
  uint64 ignored_1                 :1;
  uint64 page_ppn                  :24; // MAXPHYADDR (36) - 12
  uint64 reserved_2                :27; // must be 0
  uint64 execution_disabled        :1;
} __attribute__((__packed__)) PageTableEntry;



#endif

