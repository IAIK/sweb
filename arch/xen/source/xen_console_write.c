//----------------------------------------------------------------------
//  $Id: xen_console_write.c,v 1.1 2005/08/01 08:22:38 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_console_write.c,v $
//
//----------------------------------------------------------------------


#include "ctrl_if.h"
#include "lib.h"

/* The kernel and user-land drivers share a common transmit buffer. */
#define WBUF_SIZE     4096
#define WBUF_MASK(_i) ((_i)&(WBUF_SIZE-1))
/*
static char wbuf[WBUF_SIZE];
static unsigned int wc = 0;
static unsigned int wp = 0;
*/ /* write_cons, write_prod */

static void __xencons_tx_flush(ctrl_msg_t *);


void kcons_write(const char *s, unsigned int count)
{
    int i, j;
    ctrl_msg_t msg;
    msg.type    = CMSG_CONSOLE;
    msg.subtype = CMSG_CONSOLE_DATA;
    
    j = 0;
    
    for (i = 0; i < count; i++)
    {
        if ((msg.msg[j++] = s[i]) == '\n')
        {
            if (j >= sizeof(msg.msg))
            {
                msg.length = j;
                __xencons_tx_flush(&msg);
                j = 0;
            }
            msg.msg[j++] = '\r';
        }
        
        if (j >= sizeof(msg.msg))
        {
            msg.length = j;
            __xencons_tx_flush(&msg);
            j = 0;
        }   
    }
    
    if (j > 0)
    {   
        msg.length = j;
        __xencons_tx_flush(&msg);
    }
}   
            
        
static void __xencons_tx_flush(ctrl_msg_t *msg)
{
    while (ctrl_if_send_message_noblock(msg, 0, 0) != 0);
    
    /*
    if ( ctrl_if_send_message_noblock(&msg, 0, 0) == 0 )
        wc += sz;
    
    else if ( ctrl_if_enqueue_space_callback(&xencons_tx_flush_task) )
        break;
    */
}

/*
void kcons_write(
    struct console *c, const char *s, unsigned int count)
{
    int           i;
        
    for ( i = 0; i < count; i++ )
    {
        if ( (wp - wc) >= (WBUF_SIZE - 1) )
            break;
        if ( (wbuf[WBUF_MASK(wp++)] = s[i]) == '\n' )
            wbuf[WBUF_MASK(wp++)] = '\r';
    }

    __xencons_tx_flush();

}


static void __xencons_tx_flush(void)
{
    int sz;
    ctrl_msg_t msg;

    while ( wc != wp )
    {
        sz = wp - wc;
        if ( sz > sizeof(msg.msg) )
            sz = sizeof(msg.msg);
        if ( sz > (WBUF_SIZE - WBUF_MASK(wc)) )
        sz = WBUF_SIZE - WBUF_MASK(wc);

    msg.type    = CMSG_CONSOLE;
    msg.type = 33;
    msg.subtype = CMSG_CONSOLE_DATA;
    msg.length  = sz;
    memcpy(msg.msg, &wbuf[WBUF_MASK(wc)], sz);
    
    msg.msg[0] = 'H';
    msg.msg[1] = 'e';
    msg.msg[2] = 'l';
    msg.msg[3] = 'l';
    msg.msg[4] = 'o';
    msg.msg[5] = '\r';
    msg.msg[5] = '\n';
    msg.msg[6] = '\0';
    msg.length = 8;
        
    if ( ctrl_if_send_message_noblock(&msg, 0, 0) == 0 )
        wc += sz;
    
    else if ( ctrl_if_enqueue_space_callback(&xencons_tx_flush_task) )
        break;
    
    }
}
*/
