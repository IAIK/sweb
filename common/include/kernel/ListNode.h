// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Andreas Niederl
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * CVS Log Info for $RCSfile: ListNode.h,v $
 *
 * $Id: ListNode.h,v 1.1 2005/05/26 01:08:37 aniederl Exp $
 * $Log$
 */


#ifndef ListNode_h___
#define ListNode_h___


#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___


/**
 * class ListNode provides an abstraction to array access
 *
 */
template<typename ValueType>
class ListNode
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


  /**
   * the maximum possible index, used for returning a failure, when an index
   * is to be returned
   *
   */
  static const size_type NPOS;

  /**
   * default node array size
   *
   */
  static const size_type DEFAULT_ARRAY_SIZE;


 protected:

  /**
   * an array containing the elements of the node
   *
   */
  ValueType *data_;

  /**
   * initial size of the data array
   *
   */
  size_type initial_size_;

  /**
   * the threshold for reducing the array size
   *
   */
  size_type threshold_;

  /**
   * size of the data array
   *
   */
  size_type size_;

  /**
   * number of elements contained in the node
   *
   */
  size_type number_of_elements_;

  /**
   * the next list node
   *
   */
  ListNode<ValueType> *next_node_;

  /**
   * the previous list node
   *
   */
  ListNode<ValueType> *previous_node_;

  /**
   * backward access flag, if true the array is accessed from the upper bound
   * as border, not the number of elements as usual
   * i.e. if element with index 2 is to be accessed, then the array is indexed
   * with array size - 3
   *
   */
  bool backward_access_;

  /**
   * offset of the first element, used to speed up backward access
   *
   */
  size_type first_element_offset_;


 public:
  /**
   * default constructor for class ListNode
   * @param size initial size of the array contained in the node
   */
  ListNode(size_type size = DEFAULT_ARRAY_SIZE);


  /**
   * destructor for class ListNode
   */
  virtual ~ListNode();


  /**
   * copy constructor for class ListNode
   * @param instance is a constant reference to an object of
   * type ListNode
   */
  ListNode(const ListNode<ValueType> &instance);

  /**
   * = operator for class ListNode
   * @param instance is a reference to a ListNode object
   * @return is a reference to a ListNode object
   */
  virtual ListNode<ValueType>& operator = (
    const ListNode<ValueType> &instance);

  /**
   * checks if the next node exists
   * @return true if the next node exists
   *
   */
  bool hasNextNode() const;

  /**
   * checks if the previous node exists
   * @return true if the previous node exists
   *
   */
  bool hasPreviousNode() const;

  /**
   * returns the next node
   * @return the next node
   *
   */
  ListNode<ValueType> *getNextNode() const;

  /**
   * returns the previous node
   * @return the previous node
   *
   */
  ListNode<ValueType> *getPreviousNode() const;

  /**
   * sets the next node
   * @param next_node the next node
   *
   */
  void setNextNode(ListNode<ValueType> *next_node);

  /**
   * sets the previous node
   * @param previous_node the previous node
   *
   */
  void setPreviousNode(ListNode<ValueType> *previous_node);

  /**
   * returns the number of elements in the data array
   * @return the number of elements in the data array
   *
   */
  size_type size() const;

  /**
   * checks, if the node is empty
   * @return true, if the node is empty
   *
   */
  bool empty() const;

  /**
   * checks if the node is full (number of elements equals array size)
   * @return true if the node is full
   *
   */
  bool full() const;

  /**
   * returns the element at the given index
   * @param index the index of the asked element
   * @return the element at the given index
   *
   */
  reference get(size_type index);

  /**
   * returns the element at the given index
   * @param index the index of the asked element
   * @return the element at the given index
   *
   */
  const_reference get(size_type index) const;

  /**
   * inserts an element at the given index
   * @param index the index at which the element is to be inserted
   * @param element the element to insert
   *
   */
  void insert(size_type index, const_reference element);

  /**
   * appends an element to the node
   * @param element the element to add
   *
   */
  void append(const_reference element);

  /**
   * removes the element at the given index
   * @param index the index of the element to remove
   *
   */
  void remove(size_type index);

  /**
   * sets the backward access mode
   * @param backward_access true for backward access
   *
   */
  void setBackwardAccess(bool backward_access = true);


 private:
  /**
   * shifts the elements to the upper bound for backward access
   *
   */
  void shiftElementsToBack();

  /**
   * shifts the elements to the lower bound for normal access
   *
   */
  void shiftElementsToFront();

};


#include "ListNode.tcpp"


#endif // ListNode_h___


