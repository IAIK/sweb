//----------------------------------------------------------------------
//   $Id: RingBuffer.h,v 1.1 2005/09/16 12:47:41 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: FiFoDRBOSS.h,v $
//   Revision 1.5  2005/09/15 18:47:06  btittelbach
//   FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//   IdleThread now uses hlt instead of yield.
//
//   Revision 1.4  2005/09/15 17:51:13  nelles
//
//
//    Massive update. Like PatchThursday.
//    Keyboard is now available.
//    Each Terminal has a buffer attached to it and threads should read the buffer
//    of the attached terminal. See TestingThreads.h in common/include/kernel for
//    example of how to do it.
//    Switching of the terminals is done with the SHFT+F-keys. (CTRL+Fkeys gets
//    eaten by X on my machine and does not reach Bochs).
//    Lot of smaller modifications, to FiFo, Mutex etc.
//
//    Committing in .
//
//    Modified Files:
//    	arch/x86/source/InterruptUtils.cpp
//    	common/include/console/Console.h
//    	common/include/console/Terminal.h
//    	common/include/console/TextConsole.h common/include/ipc/FiFo.h
//    	common/include/ipc/FiFoDRBOSS.h common/include/kernel/Mutex.h
//    	common/source/console/Console.cpp
//    	common/source/console/Makefile
//    	common/source/console/Terminal.cpp
//    	common/source/console/TextConsole.cpp
//    	common/source/kernel/Condition.cpp
//    	common/source/kernel/Mutex.cpp
//    	common/source/kernel/Scheduler.cpp
//    	common/source/kernel/Thread.cpp common/source/kernel/main.cpp
//    Added Files:
//    	arch/x86/include/arch_keyboard_manager.h
//    	arch/x86/source/arch_keyboard_manager.cpp
//
//   Revision 1.3  2005/09/14 09:16:36  btittelbach
//   BugFix
//
//   Revision 1.2  2005/09/13 15:00:51  btittelbach
//   Prepare to be Synchronised...
//   kprintf_nosleep works now
//   scheduler/list still needs to be fixed
//
//   Revision 1.1  2005/09/07 00:33:52  btittelbach
//   +More Bugfixes
//   +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//----------------------------------------------------------------------

//Nice text from Jack:
//The key attribute of a ringbuffer is that it can be safely accessed by two threads
//simultaneously, one reading from the buffer and the other writing to it, without using 
//any synchronization or mutual exclusion primitives. For this to work correctly, 
//there can only be a single reader and a single writer thread. Their identities 
//cannot be interchanged.

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif

#include "mm/new.h"
#include "ArchThreads.h"
#include "assert.h"

template<class T>
class RingBuffer
{
public:
  RingBuffer(uint32 size=128);
  ~RingBuffer();

  bool get(T &c);
  void put(T c);
  void clear();
 
private:
  
  uint32 buffer_size_;
  T *buffer_;
  uint32 write_pos_;
  uint32 read_pos_;  
};

template <class T>
RingBuffer<T>::RingBuffer(uint32 size)
{  
  assert(size>1);
  buffer_size_=size;
  buffer_=new T[buffer_size_];
  write_pos_=1;
  read_pos_=0;  
}

template <class T>
RingBuffer<T>::~RingBuffer()
{
  delete[] buffer_;
}

template <class T>
void RingBuffer<T>::put(T c)
{
  uint32 old_write_pos=write_pos_;
  if (old_write_pos == read_pos_)
    return;
  buffer_[old_write_pos]=c;
  ArchThreads::testSetLock(write_pos_,(old_write_pos + 1) % buffer_size_);
}

template <class T>
void RingBuffer<T>::clear()
{
  ArchThreads::testSetLock(write_pos_,1);
  //hier könnte kurz müll gelesen werden, aber wir gehen davon aus, daß es nur einen Reader gibt
  //der natürlich nicht gleichzeitig clear und get aufrufen kann.
  ArchThreads::testSetLock(read_pos_,0);
}

template <class T>
bool RingBuffer<T>::get(T &c)
{  
  uint32 new_read_pos = (read_pos_ + 1) % buffer_size_;
  if (write_pos_ == new_read_pos) //nothing new to read
    return false;
  c = buffer_[new_read_pos];
  ArchThreads::testSetLock(read_pos_,new_read_pos);
  return true;
}

#endif
