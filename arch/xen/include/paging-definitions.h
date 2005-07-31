/**
 * $Id: paging-definitions.h,v 1.1 2005/07/31 18:31:09 nightcreature Exp $
 *
 * $Log: paging-definitions.h,v $
 *
 */

#ifndef __PAGING_DEFINITIONS_H__
#define __PAGING_DEFINITIONS_H__

#include "types.h"


#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096
#define PAGE_INDEX_OFFSET_BITS 12
#define PAGE_ADDR_MASK      0xFFFFF000

#define PAGE_PRESENT        0x00000001
#define PAGE_WRITEABLE      0x00000002
#define PAGE_USER_ACCESS    0x00000004
//#define PAGE_WRITETHROUGH   0x00000008 //reserved
//#define PAGE_DISABLE_CACHE  0x00000010 //reserved
#define PAGE_ACCESSED       0x00000020
#define PAGE_DIRTY          0x00000040
//#define PAGE_GLOBAL         0x00000100 //reserved
#define PAGE_PINNED         0x00000200
#define PAGE_2ND_CHANCE     0x00000400
#define PAGE_SWAPPED        0x00000800

#define PHYSICAL_OFFSET     0xC0000000

//------------------------------------------------------------
// Constants for page fault handling
#define ERROR_MASK            0x00000007
#define PAGE_FAULT_USER       0x00000004
#define PAGE_FAULT_WRITEABLE  0x00000002
#define PAGE_FAULT_PRESENT    0x00000001

struct page_directory_entry_4k_struct
{
  uint32 present                   :1;
	uint32 writeable                 :1;
	uint32 user_access               :1;
	uint32 write_through             :1;
	uint32 cache_disabled            :1;
	uint32 accessed                  :1;
	uint32 reserved                  :1;
	uint32 use_4_m_pages             :1;
	uint32 global_page               :1;
	uint32 avail_1                   :1;
	uint32 avail_2                   :1;
	uint32 avail_3                   :1;
	uint32 page_table_base_address   :20; 
} __attribute__((__packed__));

struct page_directory_entry_4m_struct
{
  uint32 present                   :1;
	uint32 writeable                 :1;
	uint32 user_access               :1;
	uint32 write_through             :1;
	uint32 cache_disabled            :1;
	uint32 accessed                  :1;
	uint32 dirty                     :1;
	uint32 use_4_m_pages             :1;
	uint32 global_page               :1;
	uint32 avail_1                   :1;
	uint32 avail_2                   :1;
	uint32 avail_3                   :1;
  uint32 pat                       :1;
  uint32 reserved                  :9;
  uint32 page_base_address         :10;
} __attribute__((__packed__));

union page_directory_entry_union
{
	struct page_directory_entry_4k_struct pde4k;
	struct page_directory_entry_4m_struct pde4m;
} __attribute__((__packed__));


typedef union page_directory_entry_union page_directory_entry;

struct page_table_entry_struct
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
	uint32 avail_1                   :1;
	uint32 avail_2                   :1;
	uint32 avail_3                   :1;
	uint32 page_base_address         :20; 
} __attribute__((__packed__));

typedef struct page_table_entry_struct page_table_entry;

#endif
