//----------------------------------------------------------------------
//  $Id: kprintf_xen.cpp,v 1.1 2005/08/01 08:29:13 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: kprintf_xen.cpp,v $
//
//----------------------------------------------------------------------

#include "stdarg.h"

extern "C" int printf(const char *fmt, ...); 


void kprintf(const char *fmt, ...)
{
	va_list ap;
  printf(fmt, ap);
}

void kprintfd(const char *fmt, ...)
{
	va_list ap;
  printf(fmt, ap);
}

void kprintf_nosleep(const char *fmt, ...)
{
	va_list ap;
  printf(fmt, ap);
}

void kprintfd_nosleep(const char *fmt, ...)
{
	va_list ap;
  printf(fmt, ap);
}

void kprintf_nosleep_flush(){return;}
