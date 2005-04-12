/**
 * $Id: page-table-manip.h,v 1.1 2005/04/12 17:46:44 nomenquis Exp $
 *
 * $Log:  $
 *
 */

#ifndef _PAGE_TABLE_MANIP_H_
#define _PAGE_TABLE_MANIP_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif
  extern void switchToPageTable(void *pde);
#ifdef __cplusplus
}
#endif

#endif
