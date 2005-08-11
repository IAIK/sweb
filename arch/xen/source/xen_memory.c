//----------------------------------------------------------------------
//  $Id: xen_memory.c,v 1.1 2005/08/11 16:59:10 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_memory.c,v $
//
//
//----------------------------------------------------------------------

#include "os.h"
#include "hypervisor.h"
#include "xen_memory.h"
#include "types.h"
#include "lib.h"

unsigned long *phys_to_machine_mapping;
extern char *stack;
extern char _text, _etext, _edata, _end;

static void initalisePhysToMachineMapping()
{
  uint64 map_mfn = HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list;
  //phys_to_machine_mapping = machine_to_phys(map_mfn);
  phys_to_machine_mapping = mfn_to_pfn(map_mfn);
}
