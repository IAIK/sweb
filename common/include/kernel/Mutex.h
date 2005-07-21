//----------------------------------------------------------------------
//  $Id: Mutex.h,v 1.3 2005/07/21 19:08:40 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.h,v $
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

};

#endif
