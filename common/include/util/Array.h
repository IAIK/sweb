
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
    Array() : num_elems_ ( 0 ), size_ ( 0 ), elements_(0)
    {
    }

    /*
     * Copy constructors. Copies all elements from ths to this.
     */
    Array(const Array& rhs)
      : num_elems_(rhs.num_elems_), size_(rhs.num_elems_),
        elements_(0)
    {
      if (!size_)
        return;

      elements_ = new T[size_];
      for (uint32 u = 0; u != rhs.num_elems_; ++u)
        elements_[u] = rhs.elements_[u];
    }

    ~Array()
    {
      delete[] elements_;
    }

    /**
     * Assignment operator. Copies all elements from rhs to this.
     */
    Array& operator =(const Array& rhs)
    {
      if (!rhs.num_elems_)
      {
        delete[] elements_;
        elements_ = 0;
        num_elems_ = 0;
        size_ = 0;
      }
      else
      {
        resetSize(rhs.num_elems_);
        for (uint32 u = 0; u != rhs.num_elems_; ++u)
          elements_[u] = rhs.elements_[u];
        num_elems_ = rhs.num_elems_;
      }

      return *this;
    }

    /**
     * returns number of elements in the array
     * @return the number
     */
    uint32 getNumElems() const
    {
      return num_elems_;
    }

    /**
     * returns the element at the given position
     * @param elem the element position
     * @return the element
     */
    T const &getElement ( uint32 elem ) const
    {
      return elements_[elem];
    }

    T const & operator[] (uint32 i) const
    {
      return elements_[i];
    }

    T &getElement ( uint32 elem )
    {
      return elements_[elem];
    }

    T & operator[] (uint32 i)
    {
      return elements_[i];
    }

    /**
     * appends the given element to the end of the array
     * @param element the element to add
     */
    void pushBack ( T const &element )
    {
      if ( num_elems_ + 1 >= size_ )
        resetSize ( size_*2 + 1 );

      elements_[num_elems_++] = element;
    }

    /**
     * changes the arrays length to the given size
     * @param new_size the new size
     */
    bool resetSize ( uint32 new_size )
    {
      if ( new_size <= size_ )
      {
        // perhaps some day ....
        return false;
      }

      T* new_t = new T[new_size];
      if(!new_t)
        return false;

      for ( uint32 i=0; i < num_elems_; ++i )
        new_t[i] = elements_[i];

      delete[] elements_;
      elements_ = new_t;
      size_ = new_size;

      return true;
    }

  private:

    uint32 num_elems_;
    uint32 size_;
    T *elements_;


};



#endif
