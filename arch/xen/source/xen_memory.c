//----------------------------------------------------------------------
//  $Id: xen_memory.c,v 1.4 2006/01/29 11:02:49 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_memory.c,v $
//  Revision 1.3  2006/01/20 07:20:04  nightcreature
//  updating to xen-3.0, modified sweb main to get the kernel end out of
//  ArchCommon
//
//  Revision 1.2  2005/09/28 16:35:43  nightcreature
//  main.cpp: added XenConsole (partly implemented but works) to replace TextConsole
//  in xenbuild, first batch of fixes in xen part
//
//  Revision 1.1  2005/08/11 16:59:10  nightcreature
//  replacing mm.h
//
//
//
//----------------------------------------------------------------------

#include "os.h"
#include "hypervisor.h"
#include "xen_memory.h"
#include "types.h"
#include "lib.h"
#include "paging-definitions.h"
#include "xenprintf.h"

unsigned long *physical_to_machine_mapping_;
extern char *stack;
extern char _text, _etext, _edata, _end;

//extern void initalisePhysMapping3GB(uint32 nr_pages);
//extern void initalisePhysToMachineMapping();

void initalisePhysToMachineMapping()
{
  xenprintf("mfn list: %08lx\n\n", start_info_.mfn_list);
  physical_to_machine_mapping_ = (unsigned long *) start_info_.mfn_list;
}

//----------------------------------------------------------------------

void initalisePhysMapping3GB(uint32 nr_pages)
{
  uint32 i =0;
  unsigned long vadr = 1024*1024*1024*3U;

  //TODO: This is a reather slow method of page talbe update
  //would be better to use mmu_update in batch mode

  //// FIXXME (andy, 2006-01-19 15:04:11) -> liegt nicht kernel am beginn von
  //// 3gb, darf nicht neu mappen!
  
  ;
  
// END FIXXME (andy, 2006-01-19 15:04:11)

  
  //TODO: alles erstmal readonly mappen, dann die ausnahmen einbauen
  //for(i = 0; i < nr_pages; i++)
  for(i = 0; i < 10; i++)
  {
    if ( HYPERVISOR_update_va_mapping(vadr, 
       __pte(pfn_to_machine(i) | 1), UVMF_INVLPG) )
    {
      //printk("Failed to map shared_info!!\n");
      *(int*)0=0;
    }
    //return (shared_info_t *)shared_info;
  }
}
