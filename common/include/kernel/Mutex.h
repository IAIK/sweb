//----------------------------------------------------------------------
//  $Id: Mutex.h,v 1.4 2005/07/24 17:02:59 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.h,v $
//  Revision 1.3  2005/07/21 19:08:40  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.2  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.1  2005/04/24 20:39:31  nomenquis
//  cleanups
//
//----------------------------------------------------------------------


#ifndef _MUTEX_H_
#define _MUTEX_H_


#include "types.h"
#include "List.h"

class Thread;
class Mutex
{
public:
  
  Mutex();

  void acquire();
  void release();


private:
  
  uint32 mutex_;
  List<Thread*> sleepers_;

  //this is a no no
  Mutex(Mutex const &){}
  Mutex &operator=(Mutex const&){return *this;}
};

class MutexLock
{
public:
	
  MutexLock(Mutex &m) :mutex_(m)
  {
    mutex_.acquire();
  }
  ~MutexLock()
  {
    mutex_.release();
  }
  
private:
  
  // this is a no no
  MutexLock(MutexLock const&) : mutex_(*static_cast<Mutex*>(0))
  {}
    
  Mutex &mutex_;
};

#endif
