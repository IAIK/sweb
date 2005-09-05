//----------------------------------------------------------------------
//   $Id: FiFo.h,v 1.7 2005/09/05 23:01:24 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: FiFo.h,v $
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
 
private:
  T* pos_add(T* pos_pointer, uint32 value);

  Mutex my_lock_;
  Thread *micro_cv_buffer_empty_;
  Thread *micro_cv_buffer_full_;
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
  micro_cv_buffer_full_=0;
  micro_cv_buffer_empty_=0;
}

template <class T>
FiFo<T>::~FiFo()
{
  delete[] buffer_start_;
}

template <class T>
T FiFo<T>::get()
{
  my_lock_.acquire();
  if (micro_cv_buffer_full_)
  {
    Scheduler::instance()->wake(micro_cv_buffer_full_);
    micro_cv_buffer_full_=0;
  }
  while (write_pos_ == pos_add(read_pos_,1)) //nothing new to read
  {
    //block with a real cv, pseudo cv for now
    kprintfd("FiFo::get: blocking get\n");
    micro_cv_buffer_empty_=currentThread;
    my_lock_.release();
    Scheduler::instance()->sleep();
    my_lock_.acquire();
    micro_cv_buffer_empty_=0;    
  }
  read_pos_ = pos_add(read_pos_,1);
  T element = *read_pos_;
  my_lock_.release();
  return element;
}

template <class T>
void FiFo<T>::put(T in)
{
  my_lock_.acquire();
  if (micro_cv_buffer_empty_)
  {
    Scheduler::instance()->wake(micro_cv_buffer_empty_);
    micro_cv_buffer_empty_=0;
  }
  while (pos_add(write_pos_,1) == read_pos_) //no space to write, need to read first
  {
    //block somehow, need sync mechanism for this
    kprintfd("FiFo:put: blocking put\n");
    micro_cv_buffer_full_=currentThread;
    my_lock_.release();
    Scheduler::instance()->sleep();
    my_lock_.acquire();
    micro_cv_buffer_full_=0;
  }
  
  if (read_pos_ == 0)
    read_pos_ = buffer_start_;
  
  *write_pos_=in;
  write_pos_ = pos_add(write_pos_,1);
  my_lock_.release();
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
