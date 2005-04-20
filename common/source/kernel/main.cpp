/**
 * $Id: main.cpp,v 1.3 2005/04/20 08:06:18 nomenquis Exp $
 *
 * $Log: main.cpp,v $
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
 
 
extern "C"
{
	extern char kernel_start_address;
  
	int main()
	{
    uint8 * framebuffer = (uint8*)(0x0C00B8020);
		char * start_of_text = &kernel_start_address;
    framebuffer[0] = 0x57;
    framebuffer[1] = 0x9f;
    framebuffer[2] = 0x67;
    framebuffer[3] = 0x9f;
    framebuffer[4] = 0x56;
    framebuffer[5] = 0x9f;
    
		for (;;)start_of_text++;
		return 0;
	}
	  
}
