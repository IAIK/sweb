//----------------------------------------------------------------------
//   $Id: FiFoDRBOSS.h,v 1.8 2005/09/16 15:47:41 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: FiFoDRBOSS.h,v $
//   Revision 1.7  2005/09/16 12:47:41  btittelbach
//   Second PatchThursday:
//   +KeyboardInput SyncStructure Rewrite
//   +added RingBuffer
//   +debugged FiFoDRBOSS (even though now obsolete)
//   +improved FiFo
//   +more debugging
//   Added Files:
//    	common/include/ipc/RingBuffer.h
//
//   Revision 1.6  2005/09/16 00:54:13  btittelbach
//   Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
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

//This is the Double Ring Buffered One Side Synchronised FiFo, intended for
//Input from _one_ irqHandler and Output to one or more Threads
//WARNING: the wakeup of the reciever thread IS NOT guaranteed with this FiFo 
//(it only gets more likely, the more you put in the FiFo)

#ifndef _FIFO_DRBOSS_H
#define _FIFO_DRBOSS_H

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif

#include "mm/new.h"
#include "kernel/Mutex.h"
#include "kernel/Condition.h"
#include "ArchInterrupts.h"
#include "panic.h"

template<class T>
class FiFoDRBOSS
{
public:
  FiFoDRBOSS(uint32 inputb_size=1024, uint32 fallbackb_size=0, bool dont_overwrite_old=false);
  ~FiFoDRBOSS();

  //operator <<
  //operator >>

  T get();
  void put(T c);
  uint32 countElementsAhead();
  void clear( void );
  T peekAhead();
 
private:
  void putIntoFallbackBuffer(T c);
  void putIntoBuffer(T c);
  void copyFB2Buffer();

  Mutex *input_buffer_lock_;
  Condition *something_to_read_;

  uint32 fallback_buffer_size_;
  T *fallback_buffer_;
  uint32 fb_write_pos_;
  
  uint32 input_buffer_size_;
  T *input_buffer_;
  uint32 ib_write_pos_;
  uint32 ib_read_pos_;  

  bool dont_overwrite_old_;
};

template <class T>
FiFoDRBOSS<T>::FiFoDRBOSS(uint32 inputb_size, uint32 fallbackb_size, bool dont_overwrite_old)
{
  if (inputb_size == 0)
    input_buffer_size_=1024;
  else
    input_buffer_size_=inputb_size;
  if (fallbackb_size == 0)
    fallback_buffer_size_ = inputb_size/8;
  else
    fallback_buffer_size_ = fallbackb_size;
  
  input_buffer_=new T[input_buffer_size_];
  fallback_buffer_=new T[fallback_buffer_size_];
  fb_write_pos_=0;
  ib_write_pos_=1;
  ib_read_pos_=0;  
  input_buffer_lock_=new Mutex();
  something_to_read_=new Condition(input_buffer_lock_);
  dont_overwrite_old_ = dont_overwrite_old;
}

template <class T>
FiFoDRBOSS<T>::~FiFoDRBOSS()
{
  delete[] input_buffer_;
  delete[] fallback_buffer_;
  delete input_buffer_lock_;
  delete something_to_read_;
}

template <class T>
void FiFoDRBOSS<T>::putIntoFallbackBuffer(T c)
{
  if (fb_write_pos_ < fallback_buffer_size_)
    fallback_buffer_[fb_write_pos_++]=c;
  //afterwards, dataloss occurs
}

template <class T>
void FiFoDRBOSS<T>::putIntoBuffer(T c)
{
 if (ib_write_pos_ == ib_read_pos_)
    if (dont_overwrite_old_)
      return;
    else
    {
      //if we come to close to read pos, we move read pos ahead of us
      //Therefore we are more like a RingBuffer than a FIFO where put would block
      ib_read_pos_++;
      ib_read_pos_ %= input_buffer_size_;
    }	  
 input_buffer_[ib_write_pos_++]=c;
 ib_write_pos_ %= input_buffer_size_;
}

template <class T>
void FiFoDRBOSS<T>::copyFB2Buffer()
{
  for (uint32 k=0; k < fb_write_pos_; ++k)
    putIntoBuffer(fallback_buffer_[k]);
  fb_write_pos_=0;
}

//only put uses the fallback buffer -> so it doesn't need a lock
//input_buffer could be in use -> so if locked use fallback
template <class T>
void FiFoDRBOSS<T>::put(T c)
{
 if (ArchInterrupts::testIFSet())
   kpanict((uint8*)"FiFoDRBOSS::put: is not meant to be used outside of InterruptHandler Kontext, use FiFo instead\n");
 
 if( input_buffer_lock_->isFree() )
 {
   copyFB2Buffer();
   putIntoBuffer(c);
   something_to_read_->signalWithInterruptsOff();
 }
 else
 {
   putIntoFallbackBuffer(c);
 }
}

template <class T>
void FiFoDRBOSS<T>::clear( void )
{
  input_buffer_lock_->acquire();
  fb_write_pos_=0;
  ib_write_pos_=1;
  ib_read_pos_=0;
  input_buffer_lock_->release();
}

//now this routine could get preemtepd
template <class T>
T FiFoDRBOSS<T>::get()
{
  T ret=0;
  input_buffer_lock_->acquire();
  
  while (ib_write_pos_ == ((ib_read_pos_+1)%input_buffer_size_)) //nothing new to read
    something_to_read_->wait(); //this implicates release & acquire
  
  ib_read_pos_ = (ib_read_pos_+1) % input_buffer_size_;
  ret = input_buffer_[ib_read_pos_];
  
  input_buffer_lock_->release();
  return ret;
}

//now this routine could get preemtepd
template <class T>
T FiFoDRBOSS<T>::peekAhead()
{
  T ret=0;
  input_buffer_lock_->acquire();
  
  while (ib_write_pos_ == ((ib_read_pos_+1)%input_buffer_size_)) //nothing new to read
    something_to_read_->wait(); //this implicates release & acquire
  
  ret = input_buffer_[ib_read_pos_+1%input_buffer_size_];
  
  input_buffer_lock_->release();
  return ret;
}

template <class T>
uint32 FiFoDRBOSS<T>::countElementsAhead()
{
  uint32 count = ib_write_pos_ - ib_read_pos_;
  if (count == 0)
    return input_buffer_size_;
  else if (count > 0)
    return (count - 1);
  else  // count < 0
    return (input_buffer_size_ + count);
}

#endif /* _FIFO_DRBOSS_H */
