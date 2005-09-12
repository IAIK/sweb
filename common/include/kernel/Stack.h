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
 * CVS Log Info for $RCSfile: Stack.h,v $
 *
 * $Id: Stack.h,v 1.2 2005/09/12 23:29:06 aniederl Exp $
 * $Log: Stack.h,v $
 * Revision 1.1  2005/05/26 01:08:37  aniederl
 * initial import of List, Queue and Stack data structures
 *
 */


#ifndef Stack_h___
#define Stack_h___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "List.h"


/**
 * class Stack provides a wrapper for class List implementing a stack interface
 *
 */
template<typename ValueType>
class Stack
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
   * List used as element storage for the stack
   *
   */
  List<ValueType> *list_;


 public:
  /**
   * default constructor for class Stack
   *
   */
  Stack();


  /**
   * destructor for class Stack
   */
  virtual ~Stack();

  /**
   * copy constructor for class Stack
   * @param instance is a constant reference to an object of
   * type Stack
   */
  Stack(const Stack &instance);

  /**
   * = operator for class Stack
   * @param instance is a reference to a Stack object
   * @return is a reference to a Stack object
   */
  virtual Stack& operator = (const Stack &instance);


  /**
   * returns the size (number of elements) of the stack
   * @return the size (number of elements) of the stack
   *
   */
  size_type size() const;

  /**
   * checks, if the stack is empty
   * @return true, if the stack is empty
   *
   */
  bool empty() const;


  /**
   * returns the element on the top of the stack
   * @return the element on the top of the stack
   *
   */
  reference top();

  /**
   * returns the element on the top of the stack
   * @return the element on the top of the stack
   *
   */
  const_reference top() const;

  /**
   * pushes an element at the top of the stack
   * @param element the new element to add
   *
   */
  void push(const_reference element);

  /**
   * pops the top element from the stack
   *
   */
  void pop();

};


#include "Stack.tcpp"

#endif // Stack_h___


