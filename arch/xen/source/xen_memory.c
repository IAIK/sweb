//----------------------------------------------------------------------
//  $Id: xen_memory.c,v 1.2 2005/09/28 16:35:43 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_memory.c,v $
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

unsigned long *phys_to_machine_mapping; //FIXXEME: korrect type?
extern char *stack;
extern char _text, _etext, _edata, _end;

//extern void initalisePhysMapping3GB(uint32 nr_pages);
extern void initalisePhysToMachineMapping();

void initalisePhysToMachineMapping()
{
  uint64 map_mfn = HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list;
  //phys_to_machine_mapping = machine_to_phys(map_mfn);
  phys_to_machine_mapping = (unsigned long *) mfn_to_pfn(map_mfn);
}

//----------------------------------------------------------------------

void initalisePhysMapping3GB(uint32 nr_pages)
{
  uint32 i =0;
  uint32 mfn = 0;
  uint32 madr = 0;
  //uint32 virt_start = 3u*1024u*1024u*1024u;
  //uint32 result = 0;
  mmu_update_t entry;
  page_table_entry pte;
  
  for(i = 0; i < nr_pages; i++)
  {
    mfn = pfn_to_mfn(i);
    madr = phys_to_machine(i * PAGE_SIZE);
    madr <<= 2;
    entry.ptr = MMU_NORMAL_PT_UPDATE | madr;

    pte.present = 1;
    pte.writeable = 1;
    pte.user_access = 0;
    pte.accessed = 0;
    pte.dirty = 0;
    pte.page_base_address = mfn * PAGE_SIZE;

    //entry.val = (memory_t) pte;
    
    //entry.val = (mfn << 12) | 3;
    entry.val = (pte.page_base_address) | 3;
    
    //HYPERVISOR_mmu_update(&entry,1,&result);
  }
}
