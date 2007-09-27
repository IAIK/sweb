//----------------------------------------------------------------------
//  $Id: panic.h,v 1.1 2005/07/31 18:31:09 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: panic.h,v $
//
//----------------------------------------------------------------------


#ifndef __PANIC__H__
#define __PANIC__H__

#include "types.h"

typedef struct _stabs_out
{
  uint32      n_strx;           /* index into string table of name */
  uint8       n_type;           /* type of symbol */
  uint8       n_other;          /* misc info (usually empty) */
  uint16       n_desc;           /* description field */
  pointer     n_value;          /* value of symbol */
} stabs_out;

void kpanict( uint8 *message );

#endif
