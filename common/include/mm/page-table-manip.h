/**
 * $Id: page-table-manip.h,v 1.2 2005/05/20 12:42:56 btittelbach Exp $
 *
 * $Log: page-table-manip.h,v $
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */

#ifndef _PAGE_TABLE_MANIP_H_
#define _PAGE_TABLE_MANIP_H_

#include "types.h"
#include "paging-definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif
  extern void switchToPageTable(page_directory_entry *pde);
#ifdef __cplusplus
}
#endif

#endif
