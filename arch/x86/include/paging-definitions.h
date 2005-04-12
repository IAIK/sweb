/**
 * $Id: paging-definitions.h,v 1.1 2005/04/12 17:46:43 nomenquis Exp $
 *
 * $Log:  $
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



#endif
