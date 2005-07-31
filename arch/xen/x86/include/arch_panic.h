//----------------------------------------------------------------------
//   $Id: arch_panic.h,v 1.1 2005/07/31 17:51:33 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_panic.h,v $
//
//----------------------------------------------------------------------

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif
void arch_panic(uint8 *mesg);
#ifdef __cplusplus
}
#endif
