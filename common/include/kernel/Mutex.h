//----------------------------------------------------------------------
//  $Id: Mutex.h,v 1.2 2005/04/27 08:58:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.h,v $
//  Revision 1.1  2005/04/24 20:39:31  nomenquis
//  cleanups
//
//----------------------------------------------------------------------


#ifndef _MUTEX_H_
#define _MUTEX_H_


#include "types.h"
#include "Array.h"

class Thread;
class Mutex
{
public:
  
  Mutex();

  void Acquire();
  void Release();


private:
  
  uint32 mutex_;
  Array <Thread*> waiting_list_;

};

#endif
