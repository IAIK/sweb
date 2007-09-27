/**
 * @file List.h
 */

#ifndef LIST_H___
#define LIST_H___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___
#include "BaseList.h"
#include "ListNode.h"

/**
 * @class List provides a basic implementation of a not normal list container
 */
template<typename ValueType>
class List:public BaseList<ValueType>
{
  public:
    /**
    * default constructor for class List
    * the node size will be set to a default value
    */
    List();

    /**
     * destructor for class List
     */
    virtual ~List();

    /**
     * inserts an element at the end
     * @param element the new element to insert
     */
    void pushBack ( typename BaseList<ValueType>::const_reference element );

    /**
     * inserts an element at the front
     * @param element the new element to insert
     */
    void pushFront ( typename BaseList<ValueType>::const_reference element );

    /**
     * inserts an element at the given index
     * NOTE that the index MUST be smaller than or equal to the size of the list
     * @param index the index at which to insert the element
     * @param element the element to insert
     */
    void insert ( typename BaseList<ValueType>::size_type index, typename BaseList<ValueType>::const_reference element );

    /**
     * Removes the first element and adds it to the end.
     */
    void rotateBack();

    /**
     * Removes the last element and adds it to the front.
     */
    void rotateFront();

};


#include "List.tcpp"

#endif // LIST_H___
