/**
 * $Id: main.cpp,v 1.12 2005/04/23 16:03:40 btittelbach Exp $
 *
 * $Log: main.cpp,v $
 * Revision 1.11  2005/04/23 15:58:32  nomenquis
 * lots of new stuff
 *
 * Revision 1.10  2005/04/23 12:52:26  nomenquis
 * fixes
 *
 * Revision 1.9  2005/04/23 12:43:09  nomenquis
 * working page manager
 *
 * Revision 1.8  2005/04/22 20:14:25  nomenquis
 * fix for crappy old gcc versions
 *
 * Revision 1.7  2005/04/22 19:43:04  nomenquis
 *  more poison added
 *
 * Revision 1.6  2005/04/22 18:23:16  nomenquis
 * massive cleanups
 *
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
#include "mm/new.h"
#include "mm/PageManager.h"

extern void* kernel_end_address;

extern "C"
{
  
	void startup()
	{
   //KernelMemoryManager *kmm = new ((void*) &kernel_end_address) KernelMemoryManager(((pointer) &kernel_end_address )+sizeof(KernelMemoryManager),0x80400000);
   initializeKernelMemoryManager();

    char *array = new char[1024];
    for (uint32 c=0; c<1024; ++c)
      array[c]='X';
    
    char *array2 = new char[128];
    for (uint32 c=0; c<128; ++c)
      array2[c]='Y';

    char *array3 = new char[512];
    for (uint32 c=0; c<512; ++c)
      array3[c]='Z';

    //~ char *array2b = (char*) kmm->reallocateMemory((pointer) array2,512);
    //~ if (array2b == array2)
      //~ arch_panic("reallocate memory failed");
    
    //~ for (uint32 c=0; c<128; ++c)
      //~ if (array2b[c]!='Y')  
        //~ arch_panic("reallocate comparision failed");
      
    delete[] array;
    delete[] array3;
    //delete[] array2b;
    delete[] array2;
    
    //~ char *x;
    //~ x=(char*) 0x80200000;
    //~ uint8 * fb = (uint8*)0xC00B8000;
    //~ uint32 i=0;
    //~ while (x < (char*)0x80400000)
    //~ {
      //~ fb[i++] = *x++;
      //~ fb[i++] = 0x9f;
    //~ }
        
   //arch_panic((uint8*) "und stop");
    


  ConsoleManager *manager = new ConsoleManager(1);
    
    Console *console = manager->getActiveConsole();
    /*uint32 i,k;
    

    for (i=0;i<16;++i)
    {
      for (k=0;k<16;++k)
      {
        console->setBackgroundColor((Console::BACKGROUNDCOLORS)i);
        console->setForegroundColor((Console::FOREGROUNDCOLORS)k);
        console->setCharacter(i,k,'A');
      }
    }
    */
    console->setBackgroundColor(Console::BG_BLACK);
    console->setForegroundColor(Console::FG_GREEN);

    uint32 i;
  
    console->writeString((uint8 const*)"Blabb\n");  

    console->writeString((uint8 const*)"Blubb sagte die Katze und frasz den Hund\n");
    console->writeString((uint8 const*)"Noch ne Zeile\n");
    console->writeString((uint8 const*)"Und jetzt ne leere\n\n");
    console->writeString((uint8 const*)"Gruen rackete autobus\n");


    for (;;);
	}
  	  
}
