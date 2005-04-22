//----------------------------------------------------------------------
//   $Id: arch_panic.h,v 1.3 2005/04/22 19:43:04 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_panic.h,v $
//  Revision 1.2  2005/04/21 21:31:24  nomenquis
//  added lfb support, also we now use a different grub version
//  we also now read in the grub multiboot version
//
//  Revision 1.1  2005/04/20 15:26:35  nomenquis
//  more and more stuff actually works
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
