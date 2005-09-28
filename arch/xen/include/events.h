//----------------------------------------------------------------------
//  $Id: events.h,v 1.2 2005/09/28 15:57:31 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: events.h,v $
//  Revision 1.1  2005/07/31 18:21:59  nightcreature
//  from mini os code, used for transition, maybe removed
//
//
//----------------------------------------------------------------------


/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: events.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: 
 *              
 *        Date: Jul 2003
 * 
 * Environment: Xen Minimal OS
 * Description: deal with events
 *
 ****************************************************************************
 * $Id: events.h,v 1.2 2005/09/28 15:57:31 rotho Exp $
 ****************************************************************************
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

/* _EVENT_* are defined in xen-public/xen.h  */
#define EV_BLKDEV _EVENT_BLKDEV
/*#define EV_TIMER  _EVENT_TIMER*/
#define EV_TIMER VIRQ_TIMER
#define EV_CONSOLE VIRQ_CONSOLE
#define EV_DIE    _EVENT_DIE
#define EV_DEBUG  _EVENT_DEBUG
#define EV_NET    _EVENT_NET
#define EV_PS2    _EVENT_PS2

/*#define NR_EVS (sizeof(HYPERVISOR_shared_info->events) * 8)*/
#define NR_EVS 1024

/* ev handler status */
#define EVS_INPROGRESS	1	/* Event handler active - do not enter! */
#define EVS_DISABLED	2	/* Event disabled - do not enter! */
#define EVS_PENDING	    4	/* Event pending - replay on enable */
#define EVS_REPLAY	    8	/* Event has been replayed but not acked yet */

/* this represents a event handler. Chaining or sharing is not allowed */
typedef struct _ev_action_t {
	void (*handler)(int, struct pt_regs *);
    unsigned int status;		/* IRQ status */
    uint32 count;
} ev_action_t;

#ifdef __cplusplus
extern "C"
{
#endif
/* prototypes */
unsigned int do_event(int ev, struct pt_regs *regs);
unsigned int add_ev_action( int ev, void (*handler)(int, struct pt_regs *) );
unsigned int enable_ev_action( int ev );
unsigned int disable_ev_action( int ev );
void init_events(void);
#ifdef __cplusplus
}
#endif


#endif /* _EVENTS_H_ */
