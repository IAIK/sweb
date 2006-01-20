//----------------------------------------------------------------------
//  $Id: xen_early_console.c,v 1.1 2006/01/20 07:28:37 nightcreature Exp $
//----------------------------------------------------------------------
//  $Log: xen_early_console.c,v $
//
//----------------------------------------------------------------------

#include "types.h"
#include "xen.h"
#include "hypervisor.h"
#include "xen-public/event_channel.h"
#include "xen-public/io/console.h"
#include "paging-definitions.h"

extern char console_page_dummy[PAGE_SIZE]; //defined in head.S

xen_early_console_write(char* message, uint32 length)
{
  struct xencons_interface *console_if = 0;
  uint32 sent = 0;
  XENCONS_RING_IDX console_cons, console_prod;
  evtchn_op_t event_op_cmd;

  //wirklich noetig noch zu mappen? siehe linux sparse
  //drivers/console/xencons_ring.c line29, wir nur mit mfn_to_virt
  //gearbeitet...
  //bzw muss die eventuell immer aus der startinfo geholt werden?
  //TODO: Idee 1.) darf page nicht mapppen, siehe oben
    
  console_if = (struct xencons_interface *)console_page_dummy;
  console_prod = console_if->out_prod;
  console_cons = console_if->out_cons;

  //see drivers/xen/console/xencons_ring.c xencons_ring_send()
  //mb();
     
  while((sent < length) && ((console_prod - console_cons) < sizeof(console_if->out)))
    console_if->out[MASK_XENCONS_IDX(console_prod++, console_if->out)] =
      message[sent++];
  if(message[sent-1] == '\n')
    console_if->out[MASK_XENCONS_IDX(console_prod++, console_if->out)] = '\r';   

  //wmb();
  console_if->out_prod = console_prod;

  event_op_cmd.cmd = EVTCHNOP_send;
  event_op_cmd.u.send.port = start_info_.console_evtchn;
  HYPERVISOR_event_channel_op(&event_op_cmd);

}
