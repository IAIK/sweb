//----------------------------------------------------------------------
//   $Id: FiFo.h,v 1.2 2005/04/27 09:22:41 nomenquis Exp $
//----------------------------------------------------------------------
//   $Log: FiFo.h,v $
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


  //mutex my_lock;
  T* buffer_start_;
  T* buffer_end_;
  T* write_pos_; //position of next to write element
  T* read_pos_; //position of next to read element
};


template <class T>
FiFo<T>::FiFo(uint32 buffer_size)
{
  buffer_start_ = new T[buffer_size+1];
  buffer_end_ = &buffer_start_[buffer_size];
  write_pos_=buffer_start_+1;
  read_pos_=buffer_start_;
}

template <class T>
FiFo<T>::~FiFo()
{
  delete[] buffer_start_;
}

template <class T>
T FiFo<T>::get()
{
  //mylock.acquire();
  while (write_pos_ == pos_add(read_pos_,1)) //nothing new to read
  {kprintf("blocking get\n");} //block somehow, need sync mechanism for this, busy waiting for now

  read_pos_ = pos_add(read_pos_,1);
  T element = *read_pos_;
  //mylock.release();
  return element;
}

template <class T>
void FiFo<T>::put(T in)
{
  //mylock.acquire();
  while (pos_add(write_pos_,1) == read_pos_) //no space to write, need to read first
  {kprintf("blocking put\n");} //block somehow, need sync mechanism for this, busy waiting for now
  
  if (read_pos_ == 0)
    read_pos_ = buffer_start_;
  
  *write_pos_=in;
  write_pos_ = pos_add(write_pos_,1);
  //mylock.release();
}

template <class T>
T* FiFo<T>::pos_add(T* pos_pointer, uint32 value)
{
  pos_pointer+= value; //nice pointer arithmetic, accepts + and - values
  if (pos_pointer > buffer_end_)
    pos_pointer = buffer_start_ + (( ((pointer) pos_pointer ) - ((pointer) buffer_end_ ) )/sizeof(T));
  else if (pos_pointer < buffer_start_)
    pos_pointer = buffer_end_ - (( ((pointer) buffer_start_ ) - ((pointer) pos_pointer ) )/sizeof(T) - 1);
  return pos_pointer;
}


#endif /* _FIFO_H */
