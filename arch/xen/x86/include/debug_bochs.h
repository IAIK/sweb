//----------------------------------------------------------------------
//   $Id: debug_bochs.h,v 1.1 2005/07/31 17:53:48 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: debug_bochs.h,v $
//
//----------------------------------------------------------------------
///dummy implementation ...no bochs when using xen

#ifndef _DEBUG_BOCHS_H_
#define _DEBUG_BOCHS_H_
#include "types.h"

void writeChar2Bochs( uint8 char2Write );
void writeLine2Bochs( const uint8 * line2Write );

#endif
