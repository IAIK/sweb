#pragma once

#include "types.h"

#define PAGE_MAP_LEVEL_4_ENTRIES 512
#define PAGE_DIR_POINTER_TABLE_ENTRIES 512
#define PAGE_DIR_ENTRIES 512
#define PAGE_TABLE_ENTRIES 512
#define PAGE_SIZE 4096

#define PAGE_INDEX_OFFSET_BITS 12



// Constants for page fault handling
#define ERROR_MASK            0x00000007
#define PAGE_FAULT_USER       0x00000004
#define PAGE_FAULT_WRITEABLE  0x00000002
#define PAGE_FAULT_PRESENT    0x00000001

struct [[gnu::packed]] PageMapLevel4Entry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 ignored_3                 :1;
  uint64 size                      :1; // must be 0
  uint64 ignored_2                 :4;
  uint64 page_ppn                  :28;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :11;
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageMapLevel4Entry) == 8, "PageMapLevel4Entry is not 64 bit");

using PageMapLevel4 = PageMapLevel4Entry[PAGE_MAP_LEVEL_4_ENTRIES];

struct [[gnu::packed]] PageDirPointerTablePageDirEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 ignored_3                 :1;
  uint64 size                      :1; // 0 means page directory mapped
  uint64 ignored_2                 :4;
  uint64 page_ppn                  :28;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :11;
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageDirPointerTablePageDirEntry) == 8, "PageDirPointerTablePageDirEntry is not 64 bit");

struct [[gnu::packed]] PageDirPointerTablePageEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 dirty                     :1;
  uint64 size                      :1; // 1 means 1gb page mapped
  uint64 global                    :1;
  uint64 ignored_2                 :3;
  uint64 pat                       :1;
  uint64 reserved_2                :17; // must be 0
  uint64 page_ppn                  :10;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :11;
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageDirPointerTablePageEntry) == 8, "PageDirPointerTablePageEntry is not 64 bit");

union [[gnu::packed]] PageDirPointerTableEntry
{
  struct PageDirPointerTablePageDirEntry pd;
  struct PageDirPointerTablePageEntry page;
};

using PageDirPointerTable = PageDirPointerTableEntry[PAGE_DIR_POINTER_TABLE_ENTRIES];

struct [[gnu::packed]] PageDirPageTableEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 ignored_3                 :1;
  uint64 size                      :1; // 0 means page table mapped
  uint64 ignored_2                 :4;
  uint64 page_ppn                  :28;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :11;
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageDirPageTableEntry) == 8, "PageDirPageTableEntry is not 64 bit");

struct [[gnu::packed]] PageDirPageEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 dirty                     :1;
  uint64 size                      :1;  // 1 means 2mb page mapped
  uint64 global                    :1;
  uint64 ignored_2                 :3;
  uint64 pat                       :1;
  uint64 reserved_2                :8; // must be 0
  uint64 page_ppn                  :19;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :11;
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageDirPageEntry) == 8, "PageDirPageEntry is not 64 bit");

union [[gnu::packed]] PageDirEntry
{
  struct PageDirPageTableEntry pt;
  struct PageDirPageEntry page;
};

using PageDir = PageDirEntry[PAGE_DIR_ENTRIES];

struct [[gnu::packed]] PageTableEntry
{
  uint64 present                   :1;
  uint64 writeable                 :1;
  uint64 user_access               :1;
  uint64 write_through             :1;
  uint64 cache_disabled            :1;
  uint64 accessed                  :1;
  uint64 dirty                     :1;
  uint64 size                      :1; // PAT bit
  uint64 global                    :1;
  uint64 ignored_2                 :3;
  uint64 page_ppn                  :28;
  uint64 reserved_1                :12; // must be 0
  uint64 ignored_1                 :7;
  uint64 protection_key            :4; // https://www.kernel.org/doc/html/next/core-api/protection-keys.html
  uint64 execution_disabled        :1;
};

static_assert(sizeof(PageTableEntry) == 8, "PageTableEntry is not 64 bit");

using PageTable = PageTableEntry[PAGE_TABLE_ENTRIES];

struct [[gnu::packed]] VAddr
{
    union [[gnu::packed]]
    {
        uint64 addr;

        struct [[gnu::packed]]
        {
            uint64 offset   : 12;
            uint64 pti      : 9;
            uint64 pdi      : 9;
            uint64 pdpti    : 9;
            uint64 pml4i    : 9;
            uint64 reserved : 16;
        };
    };
};
