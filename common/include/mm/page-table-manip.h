/**
 * @file page-table-manip.h
 */

#ifndef PAGE_TABLE_MANIP_H__
#define PAGE_TABLE_MANIP_H__

#include "types.h"
#include "paging-definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif
  extern void switchToPageDirectoryPointer(page_directory_entry *pde);
  extern void switchToPageDirectory(uint32 physical_page_directory_page);
#ifdef __cplusplus
}
#endif

#endif
