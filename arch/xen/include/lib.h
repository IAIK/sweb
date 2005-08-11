//----------------------------------------------------------------------
//  $Id: lib.h,v 1.2 2005/08/11 16:57:20 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: lib.h,v $
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
 *        File: lib.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: 
 *              
 *        Date: Aug 2003
 * 
 * Environment: Xen Minimal OS
 * Description: Random useful library functions, contains some freebsd stuff
 *
 ****************************************************************************
 * $Id: lib.h,v 1.2 2005/08/11 16:57:20 nightcreature Exp $
 ****************************************************************************
 *
 *-
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)stdarg.h    8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/i386/include/stdarg.h,v 1.10 1999/08/28 00:44:26 peter Exp $
 */

#ifndef _LIB_H_
#define _LIB_H_

//#include "stdarg.h"

/* variadic function support */
//typedef char *va_list;
//#define __va_size(type) \
//        (((sizeof(type) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
//#ifdef __GNUC__
//#define va_start(ap, last) \
//        ((ap) = (va_list)__builtin_next_arg(last))
//#else
//#define va_start(ap, last) \
//        ((ap) = (va_list)&(last) + __va_size(last))
//#endif
//#define va_arg(ap, type) \
//        (*(type *)((ap) += __va_size(type), (ap) - __va_size(type)))
//#define va_end(ap)

//#include "printf.h"
    /* printing */ 

#define printk  printf 
    //#define kprintf printf 
int printf(const char *fmt, ...); 
int vprintf(const char *fmt, char *ap); 
int sprintf(char *buf, const char *cfmt, ...); 
int vsprintf(char *buf, const char *cfmt, char *ap); 

#include "util/string.h"


#endif /* _LIB_H_ */
