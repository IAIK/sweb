/**
 * @file RingBuffer.h
 */
#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "new.h"
#include "ArchThreads.h"
#include "assert.h"

/**
 * @class RingBuffer
 * Nice text from Jack:
 * The key attribute of a ringbuffer is that it can be safely accessed by two threads
 * simultaneously, one reading from the buffer and the other writing to it, without using
 * any synchronization or mutual exclusion primitives. For this to work correctly,
 * there can only be a single reader and a single writer thread. Their identities
 * cannot be interchanged.
 */
template<class T>
class RingBuffer
{
  public:

    /**
     * Constuctor
     * @pre size > 1
     * @param size the size of the ringbuffer (default 128)
     * @return RingBuffer instance
     */
    RingBuffer ( uint32 size=128 );

    /**
     * Desturctor
     */
    ~RingBuffer();

    /**
     * Stores an element from the ringbuffer in the given parameter and returns if there was something to get.
     * @param c the parameter to store the element in
     * @return true if there was something to get
     */
    bool get ( T &c );

    /**
     * Puts the given element in the ringbuffer.
     * @param c the element to store
     */
    void put ( T c );

    /**
     * Clears the ringbuffer.
     */
    void clear();

  private:

    size_t buffer_size_;
    T *buffer_;
    size_t write_pos_;
    size_t read_pos_;
};

template <class T>
RingBuffer<T>::RingBuffer ( uint32 size )
{
  assert ( size>1 );
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
void RingBuffer<T>::put ( T c )
{
  size_t old_write_pos=write_pos_;
  if ( old_write_pos == read_pos_ )
    return;
  buffer_[old_write_pos]=c;
  ArchThreads::testSetLock ( write_pos_, ( old_write_pos + 1 ) % buffer_size_ );
}

template <class T>
void RingBuffer<T>::clear()
{
  ArchThreads::testSetLock ( write_pos_,1 );
  // assumed that there is only one reader who can't have called clear and get at the same time.
  // here get would return garbage.
  ArchThreads::testSetLock ( read_pos_,0 );
}

template <class T>
bool RingBuffer<T>::get ( T &c )
{
  uint32 new_read_pos = ( read_pos_ + 1 ) % buffer_size_;
  if ( write_pos_ == new_read_pos ) //nothing new to read
    return false;
  c = buffer_[new_read_pos];
  ArchThreads::testSetLock ( read_pos_,new_read_pos );
  return true;
}

#endif
