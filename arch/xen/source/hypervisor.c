//----------------------------------------------------------------------
//  $Id: hypervisor.c,v 1.1 2005/08/01 08:22:38 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: hypervisor.c,v $
//
//----------------------------------------------------------------------


/******************************************************************************
 * hypervisor.c
 * 
 * Communication to/from hypervisor.
 * 
 * Copyright (c) 2002-2003, K A Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "os.h"
#include "hypervisor.h"
#include "events.h"
#include "types.h"
#include "stdarg.h"
#include "lib.h"
#include "synch_bitops.h"


static unsigned long event_mask = 0;
static unsigned long ev_err_count;
extern int console_ready;

void do_hypervisor_callback(struct pt_regs *regs)
{
    unsigned long l1, l2;
    unsigned int l1i, l2i, port;
    unsigned long flags;
    char buffer[256];
    shared_info_t *s = HYPERVISOR_shared_info;
    
    local_irq_save(flags);
    
    while (s->vcpu_data[0].evtchn_upcall_pending) {
        s->vcpu_data[0].evtchn_upcall_pending = 0;
        /* NB. No need for a barrier here -- XCHG is a barrier on x86. */
        l1 = xchg(&s->evtchn_pending_sel, 0);
        while ((l1i = ffs(l1)) != 0) {
            l1i--;
            l1 &= ~(1 << l1i);

            l2 = s->evtchn_pending[l1i] & ~s->evtchn_mask[l1i];
            while ((l2i = ffs(l2)) != 0) {
                l2i--;
                l2 &= ~(1 << l2i);

                port = (l1i << 5) + l2i;
                do_event ((int)port, regs);
                synch_clear_bit(port, &s->evtchn_pending[0]);
            }
        }
    }
    
    local_irq_restore(flags);
}

void enable_hypervisor_event(unsigned int ev)
{
   
    set_bit(ev, &event_mask);
    set_bit(ev, &HYPERVISOR_shared_info->evtchn_mask[0]);
    if ( HYPERVISOR_shared_info->vcpu_data[0].evtchn_upcall_pending)
        do_hypervisor_callback(NULL);
}

void disable_hypervisor_event(unsigned int ev)
{
    clear_bit(ev, &event_mask);
    clear_bit(ev, &HYPERVISOR_shared_info->evtchn_mask[0]);
}

void ack_hypervisor_event(unsigned int ev)
{
    if ( !(event_mask & (1<<ev)) )
        atomic_inc((atomic_t *)&ev_err_count);
    set_bit(ev, &HYPERVISOR_shared_info->evtchn_mask[0]);
}
