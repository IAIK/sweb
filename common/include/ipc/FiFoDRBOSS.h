//----------------------------------------------------------------------
//   $Id: FiFoDRBOSS.h,v 1.2 2005/09/13 15:00:51 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: FiFoDRBOSS.h,v $
//   Revision 1.1  2005/09/07 00:33:52  btittelbach
//   +More Bugfixes
//   +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//----------------------------------------------------------------------

//This is the Double Ring Buffered One Side Synchronised FiFo, intended for
//Input from _one_ irqHandler and Output to one or more Threads

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
#include "assert.h"

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
 if (dont_overwrite_old_ && ((ib_write_pos_ + 1) % input_buffer_size_ == ib_read_pos_))
   return;
 input_buffer_[ib_write_pos_++]=c;
 ib_write_pos_ %= input_buffer_size_;
  if (!dont_overwrite_old_ && ((ib_write_pos_ + 1) % input_buffer_size_ == ib_read_pos_))
  {
    //if we come to close to read pos, we move read pos ahead of us
    //Therefore we are more like a RingBuffer than a FIFO where put would block
    ib_read_pos_++;
    ib_read_pos_ %= input_buffer_size_;
  }
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
 assert(ArchInterrupts::testIFSet()==false);
  
 if (input_buffer_lock_->isFree())
 {
   copyFB2Buffer();
   putIntoBuffer(c);
   input_buffer_lock_->acquire();
   something_to_read_->signal(); //to proudly protect the list and signal the threads
   input_buffer_lock_->release();
 }
 else
 {
   putIntoFallbackBuffer(c);
 }
}

//now this routine could get preemtepd
template <class T>
T FiFoDRBOSS<T>::get()
{
  T ret=0;
  input_buffer_lock_->acquire();
  
  while (ib_write_pos_ == ((ib_read_pos_+1)%input_buffer_size_)) //nothing new to read
    something_to_read_->wait(); //this implicates release & acquire
  
  ret = input_buffer_[ib_read_pos_++];
  ib_read_pos_ %= input_buffer_size_;
  
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
