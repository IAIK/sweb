
//
// CVS Log Info for $RCSfile: assert.h,v $
//
// $Id: assert.h,v 1.1 2005/05/10 15:27:54 davrieb Exp $
// $Log$
//

#ifndef ASSSERT_H__
#define ASSSERT_H__

#include "types.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define assert(condition) sweb_assert(condition,__LINE__,__FILE__);

void sweb_assert(uint32 condition, uint32 line, char* file);

#ifdef __cplusplus
}
#endif

#endif
