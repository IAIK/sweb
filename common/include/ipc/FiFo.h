//----------------------------------------------------------------------
//   $Id: FiFo.h,v 1.11 2005/09/15 17:51:13 nelles Exp $
//----------------------------------------------------------------------
//   $Log: FiFo.h,v $
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

template<class T>
class FiFo
{
public:
  FiFo(uint32 buffer_size);
  ~FiFo();

  //operator <<
  //operator >>

  T get();
  void put(T in);
  uint32 countElementsAhead();
 
private:
  T* pos_add(T* pos_pointer, uint32 value);

  Mutex *my_lock_;
  Condition *buffer_not_empty_;
  Condition *buffer_not_full_;
  T* buffer_start_;
  T* buffer_end_;
  T* write_pos_; //position of next to write element
  T* read_pos_; //position of next to read element
};


template <class T>
FiFo<T>::FiFo(uint32 buffer_size)
{
  kprintfd("Created Fifo with Buffer size %d\n",buffer_size);
  buffer_start_ = new T[buffer_size+1];
  buffer_end_ = &buffer_start_[buffer_size];
  write_pos_=buffer_start_+1;
  read_pos_=buffer_start_;
  my_lock_=new Mutex();
  buffer_not_empty_=new Condition(my_lock_);
  buffer_not_full_=new Condition(my_lock_);
}

template <class T>
FiFo<T>::~FiFo()
{
  delete[] buffer_start_;
}

template <class T>
T FiFo<T>::get()
{
  my_lock_->acquire();
  buffer_not_full_->signal();
  while (write_pos_ == pos_add(read_pos_,1)) //nothing new to read
  {
    //block with a real cv, pseudo cv for now
    kprintfd("FiFo::get: blocking get\n");
    buffer_not_empty_->wait();
  }
  read_pos_ = pos_add(read_pos_,1);
  T element = *read_pos_;
  my_lock_->release();
  return element;
}

template <class T>
void FiFo<T>::put(T in)
{
  my_lock_->acquire();
  buffer_not_empty_->signal();
  while (pos_add(write_pos_,1) == read_pos_) //no space to write, need to read first
  {
    //block somehow, need sync mechanism for this
    kprintfd("FiFo:put: blocking put\n");
    buffer_not_full_->wait();
  }
  
  if (read_pos_ == 0)
    read_pos_ = buffer_start_;
  
  *write_pos_=in;
  write_pos_ = pos_add(write_pos_,1);
  my_lock_->release();
}

template <class T>
uint32 FiFo<T>::countElementsAhead()
{
  my_lock_->acquire();
  uint32 buffer_size = (((pointer) buffer_end_) - ((pointer) buffer_start_)) / sizeof(T);
  pointer count = (((pointer) write_pos_) - ((pointer) read_pos_)) / sizeof(T);
  my_lock_->release();
  if (count == 0)
    return buffer_size;
  else if (count > 0)
    return (count - 1);
  else  // count < 0
    return (buffer_size + count);
}

template <class T>
T* FiFo<T>::pos_add(T* pos_pointer, uint32 value)
{
  pos_pointer+= value; //nice pointer arithmetic, accepts + and - values
  if (pos_pointer > buffer_end_)
    pos_pointer = buffer_start_ + (( ((pointer) pos_pointer ) - ((pointer) buffer_end_ ) )/sizeof(T) - 1);
  else if (pos_pointer < buffer_start_)
    pos_pointer = buffer_end_ - (( ((pointer) buffer_start_ ) - ((pointer) pos_pointer ) )/sizeof(T) + 1);
  return pos_pointer;
}

#endif /* _FIFO_H */
