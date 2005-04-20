/**
 * $Id: main.cpp,v 1.2 2005/04/20 06:39:11 nomenquis Exp $
 *
 * $Log: main.cpp,v $
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
		char * start_of_text = &kernel_start_address;
		for (;;)start_of_text++;
		return 0;
	}
	  
}
