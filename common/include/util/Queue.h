/**
 * @file Queue.h
 */

#ifndef Queue_h___
#define Queue_h___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "List.h"

/**
 * @class Queue provides a wrapper for class List implementing a basic queue
 * interface
 */
template<typename ValueType>
class Queue
{
  public:

    /**
     * defines the type with which quantities are measured
     */
    typedef uint32 size_type;

    typedef ValueType value_type;

    typedef ValueType* pointer;

    typedef ValueType& reference;

    typedef const ValueType& const_reference;


  protected:

    /**
     * List used as element storage for the queue
     */
    List<ValueType> *list_;

  public:
    /**
     * default constructor for class Queue
     */
    Queue();

    /**
     * destructor for class Queue
     */
    virtual ~Queue();

    /**
     * copy constructor for class Queue
     * @param instance is a constant reference to an object of
     * type Queue
     */
    Queue ( const Queue<ValueType> &instance );

    /**
     * = operator for class Queue
     * @param instance is a reference to a Queue object
     * @return is a reference to a Queue object
     */
    virtual Queue<ValueType>& operator = ( const Queue<ValueType> &instance );

    /**
     * returns the size (number of elements) of the queue
     * @return the size (number of elements) of the queue
     *
     */
    size_type size() const;

    /**
     * checks, if the queue is empty
     * @return true, if the queue is empty
     */
    bool empty() const;

    /**
     * returns the first element
     * @return the first element
     */
    reference front();

    /**
     * returns the first element
     * @return the first element
     */
    const_reference front() const;

    /**
     * returns the last element
     * @return the last element
     */
    reference back();

    /**
     * returns the last element
     * @return the last element
     */
    const_reference back() const;

    /**
     * inserts an element at the end of the queue
     * @param element the new element to insert
     */
    void push ( const_reference element );

    /**
     * removes the first element
     */
    void pop();

};


#include "Queue.tcpp"

#endif // Queue_h___
