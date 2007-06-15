//----------------------------------------------------------------------
//  $Id: SpinLock.h,v 1.4 2005/10/26 11:17:40 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: SpinLock.h,v $
//  Revision 1.3  2005/10/24 21:28:04  nelles
//
//   Fixed block devices. I think.
//
//   Committing in .
//
//   Modified Files:
//
//   	arch/x86/include/arch_bd_ata_driver.h
//   	arch/x86/source/InterruptUtils.cpp
//   	arch/x86/source/arch_bd_ata_driver.cpp
//   	arch/x86/source/arch_bd_ide_driver.cpp
//   	arch/xen/source/arch_bd_ide_driver.cpp
//
//   	common/source/kernel/SpinLock.cpp
//   	common/source/kernel/Thread.cpp utils/bochs/bochsrc
//
//  Revision 1.2  2005/09/26 13:56:55  btittelbach
//  +doxyfication
//  +SchedulerClass upgrade
//
//  Revision 1.1  2005/08/07 16:54:21  btittelbach
//  SpinLock++ with Yield instead of BusyWaiting
//
//----------------------------------------------------------------------


#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "types.h"
#include "../util/List.h"

class Thread;
  

//-----------------------------------------------------------
/// SpinLock Class
///
/// WikiPedia about SpinLocks:
/// @url http://en.wikipedia.org/wiki/Spinlock
/// In software engineering, a spinlock is a lock where the thread simply waits in a loop ("spins")
/// repeatedly checking until the lock becomes available. This is also known as "busy waiting" 
/// because the thread remains active but isn't performing a useful task. 
/// 
/// In Sweb we use SpinLocks in situations where we cannot allocate any kernel memory
/// i.e. because we are locking the KMM itself.
/// The SpinLock however is not meant to be used in a context where the InterruptFlag is not set,
/// because as with any lock, no taskswitch can happen and deadlock would occur if acquiring the SpinLock should
/// not succeed in an IF==0 context.
/// Also, in sweb, the Spinlock uses yield()s instead of a simple do-nothing-loop
class SpinLock
{
public:
  
  SpinLock();
//-----------------------------------------------------------
/// acquires the SpinLock.
///
  void acquire();
//-----------------------------------------------------------
/// releases the SpinLock.
///
  void release();

//-----------------------------------------------------------
/// allows you to check if the SpinLock is set or not.
/// trust the return value only if the SpinLock can't be acquired or releases
/// when you're not loocking. (= only use in atomic state)
  bool isFree();

private:
  // dont't use any stuff that needs memory allocation here
  // since the spinlock is used by the KernelMemoryManager
  uint32 nosleep_mutex_;

  //this is a no no
  SpinLock(SpinLock const &){}
  SpinLock &operator=(SpinLock const&){return *this;}
};

#endif
