/**
 * $Id: main.cpp,v 1.6 2005/04/22 18:23:16 nomenquis Exp $
 *
 * $Log: main.cpp,v $
 * Revision 1.5  2005/04/22 17:21:41  nomenquis
 * added TONS of stuff, changed ZILLIONS of things
 *
 * Revision 1.4  2005/04/21 21:31:24  nomenquis
 * added lfb support, also we now use a different grub version
 * we also now read in the grub multiboot version
 *
 * Revision 1.3  2005/04/20 08:06:18  nomenquis
 * the overloard (thats me) managed to get paging with 4m pages to work.
 * kernel is now at 2g +1 and writes something to the fb
 * w00t!
 *
 * Revision 1.2  2005/04/20 06:39:11  nomenquis
 * merged makefile, also removed install from default target since it does not work
 *
 * Revision 1.1  2005/04/12 18:42:51  nomenquis
 * changed a zillion of iles
 *
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */
 
 
#include "types.h"
#include "multiboot.h"
#include "arch_panic.h"
#include "paging-definitions.h"
#include "console/ConsoleManager.h"

extern "C"
{
	int main()
	{
    ConsoleManager *manager = new ConsoleManager(1);
    for (;;);
	}
	  
}
