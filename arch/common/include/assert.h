
//
// CVS Log Info for $RCSfile: assert.h,v $
//
// $Id: assert.h,v 1.1 2005/05/31 20:25:28 btittelbach Exp $
// $Log: assert.h,v $
// Revision 1.1  2005/05/10 15:27:54  davrieb
// move assert to util/assert.h
//
//

#ifndef ASSSERT_H__
#define ASSSERT_H__

#include "types.h"




#ifdef __cplusplus
extern "C"
{
#endif


#define assert(condition) sweb_assert(condition,__LINE__,__FILE__);
#define prenew_assert(condition) pre_new_sweb_assert(condition,__LINE__,__FILE__);
  
void sweb_assert(uint32 condition, uint32 line, char* file);
void pre_new_sweb_assert(uint32 condition, uint32 line, char* file);

#ifdef __cplusplus
}
#endif

#endif
