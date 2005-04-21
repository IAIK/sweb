/**
 * $Id: main.cpp,v 1.4 2005/04/21 21:31:24 nomenquis Exp $
 *
 * $Log: main.cpp,v $
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

extern "C"
{
	extern char kernel_start_address;
  extern multiboot_info_t * multi_boot_structure_pointer;
  extern page_directory_entry kernel_page_directory_start[];

	int main()
	{
    multiboot_info_t * grub_multi = (multiboot_info_t*)(((uint32)multi_boot_structure_pointer)+1024*1024*1024*3);
    if (!grub_multi)
    {
      arch_panic("No multiboot infos found, THIS IS FATAL");
    }
    
    if (grub_multi->flags & 1)
    {
      // we have memory info;
      //arch_panic("Have memory infos");
    }
    if (grub_multi->flags & 1<<11)
    {
	    volatile char *lfb_ptr = (char*)(764*1024*1024*4);
      uint32 i;
      for (i=0;i<4*1024*1024;++i)
      {
        lfb_ptr[i] = i%256;
      }
      arch_panic("Have lfb and am done, too bad no one will see this");
    }
    arch_panic("Do not have lfb");
		return 0;
	}
	  
}
