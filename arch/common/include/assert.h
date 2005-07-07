
//
// CVS Log Info for $RCSfile: assert.h,v $
//
// $Id: assert.h,v 1.2 2005/07/07 14:56:53 davrieb Exp $
// $Log: assert.h,v $
// Revision 1.1  2005/05/31 20:25:28  btittelbach
// moved assert to where it belongs (arch) and created nicer version
//
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


#define assert(cond) (cond) ? (void)0 : sweb_assert( #cond ,__LINE__,__FILE__);
#define prenew_assert(condition) pre_new_sweb_assert(condition,__LINE__,__FILE__);
  
void sweb_assert(const char *condition, uint32 line, const char* file);
void pre_new_sweb_assert(uint32 condition, uint32 line, char* file);

#ifdef __cplusplus
}
#endif

#endif
