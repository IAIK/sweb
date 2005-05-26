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
 * CVS Log Info for $RCSfile: Queue.h,v $
 *
 * $Id: Queue.h,v 1.1 2005/05/26 01:08:37 aniederl Exp $
 * $Log$
 */


#ifndef Queue_h___
#define Queue_h___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "ListNode.h"

/**
 * class Queue provides a basic queue implementation
 *
 */
template<typename ValueType>
class Queue
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
   * default node size
   *
   */
  static const size_type DEFAULT_NODE_SIZE;


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
   * number of nodes contained in the queue
   *
   */
  size_type number_of_nodes_;

  /**
   * the number of elements contained in the queue
   *
   */
  size_type number_of_elements_;

  /**
   * index of the first element
   *
   */
  size_type first_element_index_;

  /**
   * index of the last element
   *
   */
  size_type last_element_index_;


 public:
  /**
   * default constructor for class Queue
   * @param node_size the size of the queue nodes
   */
  Queue(size_type node_size = DEFAULT_NODE_SIZE);


  /**
   * destructor for class Queue
   */
  virtual ~Queue();

  /**
   * copy constructor for class Queue
   * @param instance is a constant reference to an object of
   * type Queue
   */
  Queue(const Queue<ValueType> &instance);

  /**
   * = operator for class Queue
   * @param instance is a reference to a Queue object
   * @return is a reference to a Queue object
   */
  virtual Queue<ValueType>& operator = (const Queue<ValueType> &instance);


  /**
   * returns the size (number of elements) of the queue
   * @return the size (number of elements) of the queue
   *
   */
  size_type size() const;

  /**
   * checks, if the queue is empty
   * @return true, if the queue is empty
   *
   */
  bool empty() const;


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
   * inserts an element at the end of the queue
   * @param element the new element to insert
   *
   */
  void push(const_reference element);

  /**
   * removes the first element
   *
   */
  void pop();

};


#include "Queue.tcpp"

#endif // Queue_h___


