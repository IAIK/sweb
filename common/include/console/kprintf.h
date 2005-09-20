//----------------------------------------------------------------------
//   $Id: kprintf.h,v 1.9 2005/09/20 08:05:07 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.h,v $
//   Revision 1.8  2005/09/16 15:47:41  btittelbach
//   +even more KeyboardInput Bugfixes
//   +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//   +Thread now remembers its Terminal
//   +Syscalls are USEABLE !! :-) IF=1 !!
//   +Syscalls can block now ! ;-) Waiting for Input...
//   +more other Bugfixes
//
//   Revision 1.7  2005/09/13 15:00:51  btittelbach
//   Prepare to be Synchronised...
//   kprintf_nosleep works now
//   scheduler/list still needs to be fixed
//
//   Revision 1.6  2005/07/27 10:04:26  btittelbach
//   kprintf_nosleep and kprintfd_nosleep now works
//   Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//   Revision 1.5  2005/07/24 17:02:59  nomenquis
//   lots of changes for new console stuff
//
//   Revision 1.4  2005/06/05 07:59:35  nelles
//   The kprintf_debug or kprintfd are finished
//
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
#include "types.h"

void kprintf(const char *fmt, ...);
void kprintfd(const char *fmt, ...);

void kprintf_nosleep(const char *fmt, ...);
void kprintfd_nosleep(const char *fmt, ...);

void kprint_buffer(char *buffer, uint32 size);

void kprintf_nosleep_init();
