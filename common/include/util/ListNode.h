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
 * $Id: ListNode.h,v 1.2 2005/09/12 23:29:06 aniederl Exp $
 * $Log: ListNode.h,v $
 * Revision 1.1  2005/05/26 01:08:37  aniederl
 * initial import of List, Queue and Stack data structures
 *
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


  typedef ValueType value_type;

  typedef ValueType* pointer;

  typedef ValueType& reference;

  typedef const ValueType& const_reference;


 protected:

  /**
   * the element of the node
   *
   */
  ValueType element_;

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


 public:
  /**
   * default constructor for class ListNode
   *
   */
  ListNode();

  /**
   * constructor for class ListNode
   * @param element The element for the new node
   *
   */
  ListNode(const_reference element);


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
   * returns a reference to the node element
   * @return a reference to the node element
   *
   */
  reference getElement();

  /**
   * returns a const reference to the node element
   * @return a const reference to the node element
   *
   */
  const_reference getElement() const;

  /**
   * sets the element of the node
   * @param element The element to set
   *
   */
  void setElement(const_reference element);

};


#include "ListNode.tcpp"


#endif // ListNode_h___


