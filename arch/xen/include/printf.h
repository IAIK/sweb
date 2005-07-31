//----------------------------------------------------------------------
//  $Id: printf.h,v 1.1 2005/07/31 18:31:29 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: printf.h,v $
//
//----------------------------------------------------------------------

#ifndef __XEN_PRINTF_
#define __XEN_PRINTF_

/* printing */
//#define printk  printf
//#define kprintf printf
int printf(const char *fmt, ...);
int vprintf(const char *fmt, char *ap);
int sprintf(char *buf, const char *cfmt, ...);
int vsprintf(char *buf, const char *cfmt, char *ap);

#endif
