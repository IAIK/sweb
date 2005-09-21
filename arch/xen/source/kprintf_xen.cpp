//----------------------------------------------------------------------
//  $Id: kprintf_xen.cpp,v 1.2 2005/09/21 02:18:58 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: kprintf_xen.cpp,v $
//  Revision 1.1  2005/08/01 08:29:13  nightcreature
//  what to say...
//
//
//----------------------------------------------------------------------

#include "stdarg.h"
#include "types.h"

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

void kprint_buffer(char *buffer, uint32 size){}
void kprintf_nosleep_init(){return;}
