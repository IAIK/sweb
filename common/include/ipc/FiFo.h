//----------------------------------------------------------------------
//   $Id: FiFo.h,v 1.14 2005/09/16 12:47:41 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: FiFo.h,v $
//   Revision 1.13  2005/09/16 00:54:13  btittelbach
//   Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
//   Revision 1.12  2005/09/15 18:47:06  btittelbach
//   FiFo should only be used in interruptHandler Kontext, for everything else use FiFo
//   IdleThread now uses hlt instead of yield.
//
//   Revision 1.11  2005/09/15 17:51:13  nelles
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
//    	common/include/ipc/FiFo.h common/include/kernel/Mutex.h
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
//   Revision 1.10  2005/09/13 21:24:42  btittelbach
//   Scheduler without Memory Allocation in critical context (at least in Theory)
//
//   Revision 1.9  2005/09/05 23:36:24  btittelbach
//   Typo Fix
//
//   Revision 1.8  2005/09/05 23:18:17  btittelbach
//   + Count Elements Ahead in Buffer
//
//   Revision 1.7  2005/09/05 23:01:24  btittelbach
//   Keyboard Input Handler
//   + several Bugfixes
//
//   Revision 1.6  2005/07/22 13:35:56  nomenquis
//   compilefix for typo
//
//   Revision 1.5  2005/07/21 19:33:41  btittelbach
//   Fifo blocks now, and students still have the opportunity to implement a real cv
//
//   Revision 1.4  2005/04/28 09:07:42  btittelbach
//   foobar
//
//   Revision 1.3  2005/04/28 09:06:56  btittelbach
//   locks und immer noch kein gutes blocking im fifo
//
//   Revision 1.2  2005/04/27 09:22:41  nomenquis
//   fix broken file
//
//   Revision 1.1  2005/04/26 21:38:43  btittelbach
//   Fifo/Pipe Template soweit das ohne Lock und CV zu implementiern ging
//   kprintf kennt jetzt auch chars
//
//
//----------------------------------------------------------------------

#ifndef _FIFO_H
#define _FIFO_H
#include "console/kprintf.h"

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif
//template fifo mit variablem buffer[array type]
//buffer mit zwei pointern, which wrap around (varibel)
//Blocking !!! wenn nicht lesen oder nicht schreiben geht
//Klasse sollte ein Monitor sein..

#include "mm/new.h"
#include "kernel/Mutex.h"
#include "kernel/Condition.h"
#include "kernel/Scheduler.h"

#define FIFO_NOBLOCK_PUT 1
#define FIFO_NOBLOCK_PUT_OVERWRITE_OLD 2

template<class T>
class FiFo
{
public:
  FiFo(uint32 inputb_size=512, uint8 flags=0);
  ~FiFo();

  //operator <<
  //operator >>

  void put(T c);
  T get();
  bool peekAhead(T &c);
  uint32 countElementsAhead();
  void clear( void ); 
private:
  Mutex *input_buffer_lock_;
  Condition *something_to_read_;
  Condition *space_to_write_;
 
  uint32 input_buffer_size_;
  T *input_buffer_;
  uint32 ib_write_pos_;
  uint32 ib_read_pos_;  

  uint8 flags_;
};

template <class T>
FiFo<T>::FiFo(uint32 inputb_size, uint8 flags)
{
  if (inputb_size < 2)
    input_buffer_size_=512;
  else
    input_buffer_size_=inputb_size;
  
  input_buffer_=new T[input_buffer_size_];
  ib_write_pos_=1;
  ib_read_pos_=0;  
  input_buffer_lock_=new Mutex();
  something_to_read_=new Condition(input_buffer_lock_);
  space_to_write_=new Condition(input_buffer_lock_);
  flags_=flags;
}

template <class T>
FiFo<T>::~FiFo()
{
  delete[] input_buffer_;
  delete input_buffer_lock_;
  delete something_to_read_;
  delete space_to_write_;
}

//only put uses the fallback buffer -> so it doesn't need a lock
//input_buffer could be in use -> so if locked use fallback
template <class T>
void FiFo<T>::put(T c)
{
  input_buffer_lock_->acquire();
  if (ib_write_pos_ == ib_read_pos_)
    if (flags_ & FIFO_NOBLOCK_PUT)
    {
      if (flags_ & FIFO_NOBLOCK_PUT_OVERWRITE_OLD)
        ib_read_pos_ = (ib_read_pos_ +1) % input_buffer_size_; //move read pos ahead of us
      else
      {
        input_buffer_lock_->release();
        return;
      }
    }
    else
      while (ib_write_pos_ == ib_read_pos_)
        space_to_write_->wait();
  something_to_read_->signal();
  input_buffer_[ib_write_pos_++]=c;
  ib_write_pos_ %= input_buffer_size_;
  input_buffer_lock_->release();
}

template <class T>
void FiFo<T>::clear( void )
{
  input_buffer_lock_->acquire();
  ib_write_pos_=1;
  ib_read_pos_=0;
  input_buffer_lock_->release();
}

//now this routine could get preemtepd
template <class T>
T FiFo<T>::get()
{
  T ret=0;
  input_buffer_lock_->acquire();
  
  while (ib_write_pos_ == ((ib_read_pos_+1)%input_buffer_size_)) //nothing new to read
    something_to_read_->wait(); //this implicates release & acquire
  
  space_to_write_->signal();
  ib_read_pos_ = (ib_read_pos_+1) % input_buffer_size_;
  ret = input_buffer_[ib_read_pos_];
  
  input_buffer_lock_->release();
  return ret;
}

//now this routine could get preemtepd
template <class T>
bool FiFo<T>::peekAhead(T &ret)
{
  input_buffer_lock_->acquire();
  
  if (ib_write_pos_ == ((ib_read_pos_+1)%input_buffer_size_)) //nothing new to read
  {
    input_buffer_lock_->release();
    return false;
  }
  
  ret = input_buffer_[(ib_read_pos_ + 1) % input_buffer_size_];
  input_buffer_lock_->release();
  return true;
}

template <class T>
uint32 FiFo<T>::countElementsAhead()
{
  input_buffer_lock_->acquire();
  uint32 count = ib_write_pos_ - ib_read_pos_;
  input_buffer_lock_->release();
  if (count == 0)
    return input_buffer_size_;
  else if (count > 0)
    return (count - 1);
  else  // count < 0
    return (input_buffer_size_ + count);
}


#endif /* _FIFO_H */
