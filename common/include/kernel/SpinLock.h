//----------------------------------------------------------------------
//  $Id: SpinLock.h,v 1.1 2005/08/07 16:54:21 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: SpinLock.h,v $
//----------------------------------------------------------------------


#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "types.h"
#include "List.h"

class Thread;
  
class SpinLock
{
public:
  
  SpinLock();

  void acquire();
  void release();


private:
  // dont't use any stuff that needs memory allocation here
  // since the spinlock is used by the KernelMemoryManager
  uint32 nosleep_mutex_;

  //this is a no no
  SpinLock(SpinLock const &){}
  SpinLock &operator=(SpinLock const&){return *this;}
};

#endif
