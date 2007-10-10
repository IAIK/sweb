/**
 * @file BaseList.h
 */

#ifndef BASE_LIST_H___
#define BASE_LIST_H___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "ListNode.h"

/**
 * @class BaseList provides a basic implementation of a list or sorted list container
 *
 */
template<typename ValueType>
class BaseList
{
  public:

    /**
     * defines the type with which quantities are measured
     *
     */
    typedef uint32 size_type;

    typedef ValueType value_type;

    typedef ValueType* pointer;

    typedef ValueType& reference;

    typedef const ValueType& const_reference;


  protected:


    /**
     * pointer to the first node
     *
     */
    ListNode<ValueType> *first_node_;

    /**
     * pointer to the last node
     *
     */
    ListNode<ValueType> *last_node_;

    /**
     * the number of elements contained in the list
     *
     */
    size_type number_of_elements_;


  public:
    /**
     * default constructor for class BaseList
     * the node size will be set to a default value
     */
    BaseList();

    /**
     * destructor for class BaseList
     */
    virtual ~BaseList();

    /**
     * copy constructor for class BaseList
     * @param instance is a constant reference to an object of
     * type BaseList
     */
    BaseList ( const BaseList<ValueType> &instance );

    /**
     * = operator for class BaseList
     * @param instance is a reference to a BaseList object
     * @return is a reference to a BaseList object
     */
    virtual BaseList<ValueType>& operator = ( const BaseList<ValueType> &instance );

    /**
     * returns the size (number of elements) of the list
     * @return the size (number of elements) of the list
     */
    size_type size() const;

    /**
     * checks, if the list is empty
     * @return true, if the list is empty
     */
    bool empty() const;

    /**
     * returns a reference to the element at the given index
     * @param index The index of the searched element
     * @return the element at the given index
     */
    reference operator[] ( size_type index );

    /**
     * returns a const reference to the element at the given index
     * @param index The index of the searched element
     * @return the element at the given index
     */
    const_reference operator[] ( size_type index ) const;

    /**
     * returns the first element
     * @return the first element
     * @pre !empty()
     */
    reference front();

    /**
     * returns the first element
     * @return the first element
     * @pre !empty()
     */
    const_reference front() const;

    /**
     * returns the last element
     * @return the last element
     * @pre !empty()
     */
    reference back();

    /**
     * returns the last element
     * @return the last element
     * @pre !empty()
     */
    const_reference back() const;

    /**
     * removes the last element
     */
    void popBack();

    /**
     * removes the first element
     */
    void popFront();

    /**
     * removes the element at the given index
     * @param index the index of the element to delete
     */
    void remove ( size_type index );


    /**
     * swaps the contents of the list with the given list
     * @param list the list with which the contents are swapped
     */
    void swapList ( BaseList<ValueType> &list );

    /**
     * cleares the list / ereases all elements of the list
     */
    void clear();
};

#include "BaseList.tcpp"

#endif // BASE_LIST_H___


