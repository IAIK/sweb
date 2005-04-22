//----------------------------------------------------------------------
//   $Id: kmalloc.cpp,v 1.1 2005/04/22 17:21:41 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "kmalloc.h"

static uint32 heap_start = (1024*1024*1024*2+2*1024*1024);


void* kmalloc(size_t size)
{
  void *ret = (void*)heap_start;
  heap_start += size;
  return ret;
}

void kfree(void * address)
{
  
}
