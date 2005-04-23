//----------------------------------------------------------------------
//  $Id: Thread.h,v 1.1 2005/04/23 21:27:12 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef _THREAD_H_
#define _THREAD_H_

#include "types.h"

class Thread
{
public:
  
  Thread();
  
  uint32 stack_[2048];

private:
  
  Thread(Thread const &);
  Thread &operator=(Thread const&);
};









#endif
