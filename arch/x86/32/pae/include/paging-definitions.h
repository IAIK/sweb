#pragma once

#include "types.h"

#include "EASTL/type_traits.h"

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
  size_t present                    :1;
  size_t reserved_1                 :2;  // must be 0
  size_t write_through              :1;
  size_t cache_disabled             :1;
  size_t reserved_2                 :4;  // must be 0
  size_t ignored_3                  :1;
  size_t ignored_2                  :1;
  size_t ignored_1                  :1;
  size_t page_ppn                   :24; // MAXPHYADDR (36) - 12
  size_t reserved_3                 :28; // must be 0

  using supports_writeable = eastl::false_type;
  using supports_user_access = eastl::false_type;
} __attribute__((__packed__)) PageDirPointerTableEntry;

static_assert(sizeof(PageDirPointerTableEntry) == 8, "PageDirPointerTablePageEntry is not 64 bit");

using PageDirPointerTable = PageDirPointerTableEntry[PAGE_DIRECTORY_POINTER_TABLE_ENTRIES];

static_assert(sizeof(PageDirPointerTable) == 32, "PageDirPointerTable is not 32 byte");

struct PageDirPageTableEntry
{
  size_t present                   :1;
  size_t writeable                 :1;
  size_t user_access               :1;
  size_t write_through             :1;
  size_t cache_disabled            :1;
  size_t accessed                  :1;
  size_t ignored_5                 :1;
  size_t size                      :1;
  size_t ignored_4                 :1;
  size_t ignored_3                 :1;
  size_t ignored_2                 :1;
  size_t ignored_1                 :1;
  size_t page_ppn                  :24; // MAXPHYADDR (36) - 12
  size_t reserved_2                :27; // must be 0
  size_t execution_disabled        :1;

  using supports_writeable = eastl::true_type;
  using supports_user_access = eastl::true_type;
} __attribute__((__packed__));

static_assert(sizeof(PageDirPageTableEntry) == 8, "PageDirPageTableEntry is not 64 bit");

struct PageDirPageEntry
{
  size_t present                   :1;
  size_t writeable                 :1;
  size_t user_access               :1;
  size_t write_through             :1;
  size_t cache_disabled            :1;
  size_t accessed                  :1;
  size_t dirty                     :1;
  size_t size                      :1;
  size_t global_page               :1;
  size_t ignored_3                 :1;
  size_t ignored_2                 :1;
  size_t ignored_1                 :1;

  size_t pat                       :1;
  size_t reserved                  :8;
  size_t page_ppn                  :15; // MAXPHYADDR (36) - 21
  size_t reserved_2                :27; // must be 0
  size_t execution_disabled        :1;

  using supports_writeable = eastl::true_type;
  using supports_user_access = eastl::true_type;
} __attribute__((__packed__));

static_assert(sizeof(PageDirPageEntry) == 8, "PageDirPageEntry is not 64 bit");

typedef union
{
  struct PageDirPageTableEntry pt;
  struct PageDirPageEntry page;
} __attribute__((__packed__)) PageDirEntry;

static_assert(sizeof(PageDirEntry) == 8, "PageDirEntry is not 64 bit");

using PageDir = PageDirEntry[PAGE_DIRECTORY_ENTRIES];

static_assert(sizeof(PageDir) == PAGE_SIZE, "PageDir is not 4096 byte");

typedef struct
{
  size_t present                   :1;
  size_t writeable                 :1;
  size_t user_access               :1;
  size_t write_through             :1;
  size_t cache_disabled            :1;
  size_t accessed                  :1;
  size_t dirty                     :1;
  size_t pat                       :1;
  size_t global_page               :1;
  size_t ignored_3                 :1;
  size_t ignored_2                 :1;
  size_t ignored_1                 :1;
  size_t page_ppn                  :24; // MAXPHYADDR (36) - 12
  size_t reserved_2                :27; // must be 0
  size_t execution_disabled        :1;

  using supports_writeable = eastl::true_type;
  using supports_user_access = eastl::true_type;
} __attribute__((__packed__)) PageTableEntry;

static_assert(sizeof(PageTableEntry) == 8, "PageTableEntry is not 64 bit");

using PageTable = PageTableEntry[PAGE_TABLE_ENTRIES];

static_assert(sizeof(PageTable) == PAGE_SIZE, "PageTable is not 4096 byte");


union VAddr
{
    size_t addr;
    struct
    {
        size_t offset :12;
        size_t pti    :9;
        size_t pdi    :9;
        size_t pdpti  :2;
    };
};

static_assert(sizeof(VAddr) == 4, "VAddr is not 32 bit");
