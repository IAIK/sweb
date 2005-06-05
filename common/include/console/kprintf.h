//----------------------------------------------------------------------
//   $Id: kprintf.h,v 1.4 2005/06/05 07:59:35 nelles Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.h,v $
//   Revision 1.3  2005/05/10 17:03:44  btittelbach
//   Kprintf Vorbereitung für Print auf Bochs Console
//   böse .o im source gelöscht
//
//   Revision 1.2  2005/04/24 20:39:31  nomenquis
//   cleanups
//
//   Revision 1.1  2005/04/24 13:33:39  btittelbach
//   skeleton of a kprintf
//
//
//----------------------------------------------------------------------

#include "stdarg.h"

void kprintf(const char *fmt, ...);
void kprintf_debug(const char *fmt, ...);
void kprintfd(const char *fmt, ...);
