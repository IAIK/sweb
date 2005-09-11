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
 * CVS Log Info for $RCSfile: List.h,v $
 *
 * $Id: List.h,v 1.5 2005/09/11 17:47:02 davrieb Exp $
 * $Log: List.h,v $
 * Revision 1.4  2005/09/11 11:30:37  davrieb
 * revert to prev revision
 *
 * Revision 1.2  2005/09/07 23:44:42  aniederl
 * changed constructor with default parameter to a separate default constructor and one (for actual usage) with parameter
 *
 * Revision 1.1  2005/05/26 01:08:37  aniederl
 * initial import of List, Queue and Stack data structures
 *
 */


#ifndef List_h___
#define List_h___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "ListNode.h"

/**
 * class List provides a basic implementation of a list container
 *
 */
template<typename ValueType>
class List
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
   * default values for internal structure parameters
   *
   */
  static const size_type NODE_SIZE;

  /**
   * the maximum possible index, used for returning a failure, when an index
   * is to be returned
   *
   */
  static const size_type NPOS = -1;


 protected:

  /**
   * (initial) size of the nodes
   *
   */
  size_type node_size_;

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
   * number of nodes contained in the list
   *
   */
  size_type number_of_nodes_;

  /**
   * the number of elements contained in the list
   *
   */
  size_type number_of_elements_;


 public:
  /**
   * default constructor for class List
   * the node size will be set to a default value
   */
  List();

  /**
   * constructor for class List
   * @param node_size the size of the list nodes
   */
  List(size_type node_size);

  /**
   * destructor for class List
   */
  virtual ~List();



  /**
   * copy constructor for class List
   * @param instance is a constant reference to an object of
   * type List
   */
  List(const List<ValueType> &instance);

  /**
   * = operator for class List
   * @param instance is a reference to a List object
   * @return is a reference to a List object
   */
  virtual List<ValueType>& operator = (const List<ValueType> &instance);


  /**
   * returns the size (number of elements) of the list
   * @return the size (number of elements) of the list
   *
   */
  size_type size() const;

  /**
   * checks, if the list is empty
   * @return true, if the list is empty
   *
   */
  bool empty() const;

  /**
   * returns a reference to the element at the given index
   * @param index The index of the searched element
   * @return the element at the given index
   *
   */
  reference operator[](size_type index);

  /**
   * returns a const reference to the element at the given index
   * @param index The index of the searched element
   * @return the element at the given index
   *
   */
  const_reference operator[](size_type index) const;

  /**
   * returns the first element
   * @return the first element
   *
   */
  reference front();

  /**
   * returns the first element
   * @return the first element
   *
   */
  const_reference front() const;

  /**
   * returns the last element
   * @return the last element
   *
   */
  reference back();

  /**
   * returns the last element
   * @return the last element
   *
   */
  const_reference back() const;


  /**
   * inserts an element at the end
   * @param element the new element to insert
   *
   */
  void pushBack(const_reference element);

  /**
   * removes the last element
   *
   */
  void popBack();

  /**
   * inserts an element at the front
   * @param element the new element to insert
   *
   */
  void pushFront(const_reference element);

  /**
   * removes the first element
   *
   */
  void popFront();

  /**
   * inserts an element at the given index
   * NOTE that the index MUST be smaller than or equal to the size of the list
   * @param index the index at which to insert the element
   * @param element the element to insert
   *
   */
  void insert(size_type index, const_reference element);

  /**
   * removes an element at the given index
   * @param index the index of the element to remove
   *
   */
  void remove(size_type index);

  /**
   * swaps the contents of the list with the given list
   * @param list the list with which the contents are swapped
   *
   */
  void swapList(List<ValueType> &list);

  /**
   * cleares the list
   *
   */
  void clear();

  /**
   * Rotate the list.
   * Removes the first element from the List and adds it to the end.
   */
  void rotate();

};


#include "List.tcpp"

#endif // List_h___


