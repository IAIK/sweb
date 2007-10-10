
/**
 * @file Array.h
 */

#ifndef ARRAY_H__
#define ARRAY_H__

#include "types.h"
#include "mm/new.h"

/**
 * @class Array
 * A basic array template
 */
template <typename T>
class Array
{
  public:

    /**
     * Constructor
     */
    Array() : num_elems_ ( 0 ), size_ ( 0 )
    {
    }

    /**
     * returns number of elements in the array
     * @return the number
     */
    uint32 getNumElems()
    {
      return num_elems_;
    }

    /**
     * returns the element at the given position
     * @param elem the element position
     * @return the element
     */
    T &getElement ( uint32 elem )
    {
      return elements_[elem];
    }

    /**
     * appends the given element to the end of the array
     * @param element the element to add
     */
    void appendElement ( T &element )
    {
      if ( num_elems_ +1 >= size_ )
      {
        resetSize ( size_*2+1 );
      }
      elements_[num_elems_++]=element;
    }

    /**
     * changes the arrays length to the given size
     * @param new_size the new size
     */
    void resetSize ( uint32 new_size )
    {
      if ( new_size <= size_ )
      {
        // perhaps some day ....
        return;
      }
      T* new_t = new T[new_size];
      uint32 i;
      for ( i=0;i<num_elems_;++i )
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
