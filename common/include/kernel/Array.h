//----------------------------------------------------------------------
//   $Id: Array.h,v 1.1 2005/04/27 09:01:20 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------


#ifndef _ARRAY_H_
#define _ARRAY_H_

#include "types.h"
#include "mm/new.h"

template <typename T>
class Array
{
public:
  
  Array(): num_elems_(0), size_(0)
  {
    
  }

  uint32 getNumElems()
  {
    return num_elems_;
  }
  
  T &getElement(uint32 elem)
  {
    return elements_[elem];
  }
  
  void appendElement(T &element)
  {
    if (num_elems_ +1 >= size_)
    {
      resetSize(size*2+1);
    }
    elements_[num_elems_++]=element;
  }
  
  void resetSize(uint32 new_size)
  {
    if (new_size <= size_)
    {
      // perhaps some day ....
      return;
    }
    T* new_t = new T[new_size];
    uint32 i;
    for (i=0;i<num_elems_;++i)
    {
      new_t[i] = elements_[i];
    }
    delete[] elements_;
    elements_ = new_t;
    size_ = new_size;
  }
  
private:
  
  uint32 num_elems_;
  uint32 size_;
  T *elements_;
  
  
};



#endif
