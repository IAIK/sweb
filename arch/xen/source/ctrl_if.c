//----------------------------------------------------------------------
//  $Id: ctrl_if.c,v 1.1 2005/08/01 08:22:38 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: ctrl_if.c,v $
//
//----------------------------------------------------------------------

#include "os.h"
#include "hypervisor.h"
#include "mm.h"
#include "events.h"
#include "time.h"
#include "types.h"
#include "stdarg.h"
#include "lib.h"
#include "ctrl_if.h"
#include <xen-public/event_channel.h>



static void ctrl_if_interrupt(int ev, struct pt_regs *regs);



static ctrl_msg_handler_t ctrl_if_rxmsg_handler[256];
static CONTROL_RING_IDX ctrl_if_tx_resp_cons;
static CONTROL_RING_IDX ctrl_if_rx_req_cons;
static int        ctrl_if_evtchn;



static void notify_via_evtchn(int port)
{
    evtchn_op_t op;
    op.cmd = EVTCHNOP_send;
    op.u.send.local_port = port;
    (void)HYPERVISOR_event_channel_op(&op);
}

static void ctrl_if_notify_controller(void)
{
    notify_via_evtchn(ctrl_if_evtchn);
}

#define get_ctrl_if() ((control_if_t *)((char *)HYPERVISOR_shared_info + 2048))
#define TX_FULL(_c)   \
    (((_c)->tx_req_prod - ctrl_if_tx_resp_cons) == CONTROL_RING_SIZE)
    

void ctrl_if_resume(void)
{
    control_if_t *ctrl_if = get_ctrl_if();

    /* Sync up with shared indexes. */
    ctrl_if_tx_resp_cons = ctrl_if->tx_resp_prod;
    ctrl_if_rx_req_cons  = ctrl_if->rx_resp_prod;

    ctrl_if_evtchn = start_info.domain_controller_evtchn;
    
    /* ** I don't have to support all that in mini-os ** */
    /*
    ctrl_if_irq    = bind_evtchn_to_irq(ctrl_if_evtchn);

    memset(&ctrl_if_irq_action, 0, sizeof(ctrl_if_irq_action));
    ctrl_if_irq_action.handler = ctrl_if_interrupt;
    ctrl_if_irq_action.name    = "ctrl-if";
    (void)setup_irq(ctrl_if_irq, &ctrl_if_irq_action);
    */
    
    /* ** Just need to register a handler ** */
    add_ev_action (ctrl_if_evtchn, ctrl_if_interrupt);
    enable_ev_action(ctrl_if_evtchn);
}


static void ctrl_if_rxmsg_default_handler(ctrl_msg_t *msg, unsigned long id)
{
    msg->length = 0;
    ctrl_if_send_response(msg);
}

void ctrl_if_init(void)
{
    int i;

    for ( i = 0; i < 256; i++ )
        ctrl_if_rxmsg_handler[i] = ctrl_if_rxmsg_default_handler;

    ctrl_if_resume();
}


void
ctrl_if_send_response(
    ctrl_msg_t *msg)
{
    control_if_t *ctrl_if = get_ctrl_if();
    ctrl_msg_t   *dmsg;

    /*
     * NB. The response may the original request message, modified in-place.
     * In this situation we may have src==dst, so no copying is required.
     */
    
    dmsg = &ctrl_if->rx_ring[MASK_CONTROL_IDX(ctrl_if->rx_resp_prod)];
    if ( dmsg != msg )
        memcpy(dmsg, msg, sizeof(*msg));

    wmb(); /* Write the message before letting the controller peek at it. */
    ctrl_if->rx_resp_prod++;

    ctrl_if_notify_controller();
}



int
ctrl_if_send_message_noblock(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd,
    unsigned long id)
{
    control_if_t *ctrl_if = get_ctrl_if();


    if ( TX_FULL(ctrl_if) )
    {
        return -EAGAIN;
    }

    msg->id = 0xFF;
    if ( hnd != NULL )
    {
        /* ** We'll do that later ** */
        /*
        for ( i = 0; ctrl_if_txmsg_id_mapping[i].fn != NULL; i++ )
            continue;
        ctrl_if_txmsg_id_mapping[i].fn = hnd;
        ctrl_if_txmsg_id_mapping[i].id = id;
        msg->id = i;
        */
    }

    memcpy(&ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)], 
           msg, 64);
    /*     
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[0] = 'H';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[1] = 'e';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[2] = 'l';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[3] = 'l';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[4] = 'o';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[5] = '\n';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].msg[6] = '\0';
    ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if->tx_req_prod)].length = 7;       
    */
    wmb(); /* Write the message before letting the controller peek at it. */
    ctrl_if->tx_req_prod++;


    ctrl_if_notify_controller();

    return 0;
}



static void ctrl_if_tx_tasklet()
{
    control_if_t *ctrl_if = get_ctrl_if();
    ctrl_msg_t   *msg;
    int           was_full = TX_FULL(ctrl_if);
    CONTROL_RING_IDX rp;
    char buffer[256];

    rp = ctrl_if->tx_resp_prod;
    rmb(); /* Ensure we see all requests up to 'rp'. */

    /*
    sprintf (buffer, "ctrl_if_tx_tasklet\n");
    kcons_write (buffer, strlen(buffer));
    */
    
    
    /*send_message_debug (0x00000000, 0xdeadbeef);*/
    while ( ctrl_if_tx_resp_cons != rp )
    {
        msg = &ctrl_if->tx_ring[MASK_CONTROL_IDX(ctrl_if_tx_resp_cons)];

        /* Execute the callback handler, if one was specified. */
        if ( msg->id != 0xFF )
        {
            /*
             ==== I'll do that later  - Richard === 
            (*ctrl_if_txmsg_id_mapping[msg->id].fn)(
                msg, ctrl_if_txmsg_id_mapping[msg->id].id);
            smp_mb(); 
            ctrl_if_txmsg_id_mapping[msg->id].fn = NULL;
            */
        }

        /*
         * Step over the message in the ring /after/ finishing reading it. As 
         * soon as the index is updated then the message may get blown away.
         */
    smp_mb();
        ctrl_if_tx_resp_cons++;
    }

    /*
    if ( was_full && !TX_FULL(ctrl_if) )
    {
        wake_up(&ctrl_if_tx_wait);
        run_task_queue(&ctrl_if_tx_tq);
    }
    */
}


static void ctrl_if_rx_tasklet()
{
    control_if_t *ctrl_if = get_ctrl_if();
    ctrl_msg_t    msg, *pmsg;
    CONTROL_RING_IDX rp;
    /* CONTROL_RING_IDX dp; */

    /* dp = ctrl_if_rxmsg_deferred_prod; */
    rp = ctrl_if->rx_req_prod;
    rmb(); /* Ensure we see all requests up to 'rp'. */

    while ( ctrl_if_rx_req_cons != rp )
    {
        pmsg = &ctrl_if->rx_ring[MASK_CONTROL_IDX(ctrl_if_rx_req_cons++)];
        memcpy(&msg, pmsg, offsetof(ctrl_msg_t, msg));

        /*
        DPRINTK("Rx-Req %u/%u :: %d/%d\n", 
                ctrl_if_rx_req_cons-1,
                ctrl_if->rx_req_prod,
                msg.type, msg.subtype);
        */
        
        if ( msg.length != 0 )
            memcpy(msg.msg, pmsg->msg, msg.length);

        /* ** I don't know what to do in that case ** */
        /*
        if ( test_bit(msg.type, 
                      (unsigned long *)&ctrl_if_rxmsg_blocking_context) )
            memcpy(&ctrl_if_rxmsg_deferred[MASK_CONTROL_IDX(dp++)],
                   &msg, offsetof(ctrl_msg_t, msg) + msg.length);
        else
        */
            (*ctrl_if_rxmsg_handler[msg.type])(&msg, 0);
    }

    /*
    if ( dp != ctrl_if_rxmsg_deferred_prod )
    {
        wmb();
        ctrl_if_rxmsg_deferred_prod = dp;
        schedule_work(&ctrl_if_rxmsg_deferred_work);
    }
    */
}


static void ctrl_if_interrupt(int ev, struct pt_regs *regs)
{

    char buffer[256];
    static int count = 0;
    
    control_if_t *ctrl_if = get_ctrl_if();
    
    if ( ctrl_if_tx_resp_cons != ctrl_if->tx_resp_prod )
        ctrl_if_tx_tasklet();

    if ( ctrl_if_rx_req_cons != ctrl_if->rx_req_prod )
        ctrl_if_rx_tasklet();
}
