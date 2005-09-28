//----------------------------------------------------------------------
//  $Id: xenprintf.h,v 1.1 2005/09/28 16:35:43 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: printf.h,v $
//  Revision 1.1  2005/07/31 18:31:29  nightcreature
//  *** empty log message ***
//
//
//----------------------------------------------------------------------

#ifndef __XEN_PRINTF_
#define __XEN_PRINTF_

/* printing */
//#define printk  printf
//#define kprintf printf
int xenprintf(const char *fmt, ...);
int xenvprintf(const char *fmt, char *ap);
int xensprintf(char *buf, const char *cfmt, ...);
int xenvsprintf(char *buf, const char *cfmt, char *ap);

#endif
