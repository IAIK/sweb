//----------------------------------------------------------------------
//  $Id: ctrl_if.h,v 1.1 2005/07/31 18:19:02 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: ctrl_if.h,v $
//
//----------------------------------------------------------------------


/******************************************************************************
 * ctrl_if.h
 * 
 * Management functions for special interface to the domain controller.
 * 
 * Copyright (c) 2004, K A Fraser
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __ASM_XEN__CTRL_IF_H__
#define __ASM_XEN__CTRL_IF_H__

#include <hypervisor.h>

#define EAGAIN 11
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

typedef control_msg_t ctrl_msg_t;

/*
 * Callback function type. Called for asynchronous processing of received
 * request messages, and responses to previously-transmitted request messages.
 * The parameters are (@msg, @id).
 *  @msg: Original request/response message (not a copy). The message can be
 *        modified in-place by the handler (e.g., a response callback can
 *        turn a request message into a response message in place). The message
 *        is no longer accessible after the callback handler returns -- if the
 *        message is required to persist for longer then it must be copied.
 *  @id:  (Response callbacks only) The 'id' that was specified when the
 *        original request message was queued for transmission.
 */
typedef void (*ctrl_msg_handler_t)(ctrl_msg_t *, unsigned long);

/*
 * Send @msg to the domain controller. Execute @hnd when a response is
 * received, passing the response message and the specified @id. This
 * operation will not block: it will return -EAGAIN if there is no space.
 * Notes:
 *  1. The @msg is copied if it is transmitted and so can be freed after this
 *     function returns.
 *  2. If @hnd is NULL then no callback is executed.
 */
int
ctrl_if_send_message_noblock(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd,
    unsigned long id);

/*
 * Send @msg to the domain controller. Execute @hnd when a response is
 * received, passing the response message and the specified @id. This
 * operation will block until the message is sent, or a signal is received
 * for the calling process (unless @wait_state is TASK_UNINTERRUPTIBLE).
 * Notes:
 *  1. The @msg is copied if it is transmitted and so can be freed after this
 *     function returns.
 *  2. If @hnd is NULL then no callback is executed.
 */
int
ctrl_if_send_message_block(
    ctrl_msg_t *msg, 
    ctrl_msg_handler_t hnd, 
    unsigned long id, 
    long wait_state);

/*
 * Send @msg to the domain controller. Block until the response is received,
 * and then copy it into the provided buffer, @rmsg.
 */
int
ctrl_if_send_message_and_get_response(
    ctrl_msg_t *msg,
    ctrl_msg_t *rmsg,
    long wait_state);

/*
 * Request a callback when there is /possibly/ space to immediately send a
 * message to the domain controller. This function returns 0 if there is
 * already space to trasnmit a message --- in this case the callback task /may/
 * still be executed. If this function returns 1 then the callback /will/ be
 * executed when space becomes available.
 */
/*
int
ctrl_if_enqueue_space_callback(
    struct tq_struct *task);
*/
/*
 * Send a response (@msg) to a message from the domain controller. This will 
 * never block.
 * Notes:
 *  1. The @msg is copied and so can be freed after this function returns.
 *  2. The @msg may be the original request message, modified in-place.
 */
void
ctrl_if_send_response(
    ctrl_msg_t *msg);

/*
 * Register a receiver for typed messages from the domain controller. The 
 * handler (@hnd) is called for every received message of specified @type.
 * Returns TRUE (non-zero) if the handler was successfully registered.
 * If CALLBACK_IN_BLOCKING CONTEXT is specified in @flags then callbacks will
 * occur in a context in which it is safe to yield (i.e., process context).
 */
#define CALLBACK_IN_BLOCKING_CONTEXT 1
int ctrl_if_register_receiver(
    uint8 type, 
    ctrl_msg_handler_t hnd,
    unsigned int flags);

/*
 * Unregister a receiver for typed messages from the domain controller. The 
 * handler (@hnd) will not be executed after this function returns.
 */
void
ctrl_if_unregister_receiver(
    uint8 type, ctrl_msg_handler_t hnd);

/* Suspend/resume notifications. */
void ctrl_if_suspend(void);
void ctrl_if_resume(void);

/* Start-of-day setup. */
void ctrl_if_init(void);

/*
 * Returns TRUE if there are no outstanding message requests at the domain
 * controller. This can be used to ensure that messages have really flushed
 * through when it is not possible to use the response-callback interface.
 * WARNING: If other subsystems are using the control interface then this
 * function might never return TRUE!
 */
int ctrl_if_transmitter_empty(void);  /* !! DANGEROUS FUNCTION !! */

/*
 * Manually discard response messages from the domain controller. 
 * WARNING: This is usually done automatically -- this function should only
 * be called when normal interrupt mechanisms are disabled!
 */
void ctrl_if_discard_responses(void); /* !! DANGEROUS FUNCTION !! */


/* 
 * Alternative instructions for different CPU types or capabilities.
 * 
 * This allows to use optimized instructions even on generic binary
 * kernels.
 * 
 * length of oldinstr must be longer or equal the length of newinstr
 * It can be padded with nops as needed.
 * 
 * For non barrier like inlines please define new variants
 * without volatile and memory clobber.
 */
#define alternative(oldinstr, newinstr, feature) 	\
	__asm__ __volatile__ ("661:\n\t" oldinstr "\n662:\n" 		     \
		      ".section .altinstructions,\"a\"\n"     	     \
		      "  .align 4\n"				       \
		      "  .long 661b\n"            /* label */          \
		      "  .long 663f\n"		  /* new instruction */ 	\
		      "  .byte %c0\n"             /* feature bit */    \
		      "  .byte 662b-661b\n"       /* sourcelen */      \
		      "  .byte 664f-663f\n"       /* replacementlen */ \
		      ".previous\n"						\
		      ".section .altinstr_replacement,\"ax\"\n"			\
		      "663:\n\t" newinstr "\n664:\n"   /* replacement */    \
		      ".previous" :: "i" (feature) : "memory")  

/*
 * Alternative inline assembly with input.
 * 
 * Pecularities:
 * No memory clobber here. 
 * Argument numbers start with 1.
 * Best is to use constraints that are fixed size (like (%1) ... "r")
 * If you use variable sized constraints like "m" or "g" in the 
 * replacement maake sure to pad to the worst case length.
 */
#define alternative_input(oldinstr, newinstr, feature, input...)		\
	__asm__ __volatile__ ("661:\n\t" oldinstr "\n662:\n"				\
		      ".section .altinstructions,\"a\"\n"			\
		      "  .align 4\n"						\
		      "  .long 661b\n"            /* label */			\
		      "  .long 663f\n"		  /* new instruction */ 	\
		      "  .byte %c0\n"             /* feature bit */		\
		      "  .byte 662b-661b\n"       /* sourcelen */		\
		      "  .byte 664f-663f\n"       /* replacementlen */ 		\
		      ".previous\n"						\
		      ".section .altinstr_replacement,\"ax\"\n"			\
		      "663:\n\t" newinstr "\n664:\n"   /* replacement */ 	\
		      ".previous" :: "i" (feature), ##input)

/*
 * Force strict CPU ordering.
 * And yes, this is required on UP too when we're talking
 * to devices.
 *
 * For now, "wmb()" doesn't actually do anything, as all
 * Intel CPU's follow what Intel calls a *Processor Order*,
 * in which all writes are seen in the program order even
 * outside the CPU.
 *
 * I expect future Intel CPU's to have a weaker ordering,
 * but I'd also expect them to finally get their act together
 * and add some real memory barriers if so.
 *
 * Some non intel clones support out of order store. wmb() ceases to be a
 * nop for these.
 */
 

/* 
 * Actually only lfence would be needed for mb() because all stores done 
 * by the kernel should be already ordered. But keep a full barrier for now. 
 */
#define X86_FEATURE_XMM2      (0*32+26)
#define mb() alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)
#define rmb() alternative("lock; addl $0,0(%%esp)", "lfence", X86_FEATURE_XMM2)

/**
 * read_barrier_depends - Flush all pending reads that subsequents reads
 * depend on.
 *
 * No data-dependent reads from memory-like regions are ever reordered
 * over this barrier.  All reads preceding this primitive are guaranteed
 * to access memory (but not necessarily other CPUs' caches) before any
 * reads following this primitive that depend on the data return by
 * any of the preceding reads.  This primitive is much lighter weight than
 * rmb() on most CPUs, and is never heavier weight than is
 * rmb().
 *
 * These ordering constraints are respected by both the local CPU
 * and the compiler.
 *
 * Ordering is not guaranteed by anything other than these primitives,
 * not even by data dependencies.  See the documentation for
 * memory_barrier() for examples and URLs to more information.
 *
 * For example, the following code would force ordering (the initial
 * value of "a" is zero, "b" is one, and "p" is "&a"):
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	b = 2;
 *	memory_barrier();
 *	p = &b;				q = p;
 *					read_barrier_depends();
 *					d = *q;
 * </programlisting>
 *
 * because the read of "*q" depends on the read of "p" and these
 * two reads are separated by a read_barrier_depends().  However,
 * the following code, with the same initial values for "a" and "b":
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	a = 2;
 *	memory_barrier();
 *	b = 3;				y = b;
 *					read_barrier_depends();
 *					x = a;
 * </programlisting>
 *
 * does not enforce ordering, since there is no data dependency between
 * the read of "a" and the read of "b".  Therefore, on some CPUs, such
 * as Alpha, "y" could be set to 3 and "x" to 0.  Use rmb()
 * in cases like thiswhere there are no data dependencies.
 **/

#define read_barrier_depends()	do { } while(0)

#ifdef CONFIG_X86_OOSTORE
/* Actually there are no OOO store capable CPUs for now that do SSE, 
   but make it already an possibility. */
#define wmb() alternative("lock; addl $0,0(%%esp)", "sfence", X86_FEATURE_XMM)
#else
#define wmb()	__asm__ __volatile__ ("": : :"memory")
#endif

#ifdef CONFIG_SMP
#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()
#define smp_read_barrier_depends()	read_barrier_depends()
#define set_mb(var, value) do { xchg(&var, value); } while (0)
#else
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#define smp_read_barrier_depends()	do { } while(0)
#define set_mb(var, value) do { var = value; barrier(); } while (0)
#endif

#define set_wmb(var, value) do { var = value; wmb(); } while (0)

#endif /* __ASM_XEN__CONTROL_IF_H__ */
