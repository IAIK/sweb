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
 * $Id: List.h,v 1.6 2005/09/12 23:29:06 aniederl Exp $
 * $Log: List.h,v $
 * Revision 1.6  2005/09/12 23:29:06  aniederl
 * complete rewrite of class List and ListNode for the sake of simplicity, stability and extendability
 *
 * Revision 1.5  2005/09/11 17:47:02  davrieb
 * add rotate()
 *
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
 * class List provides a basic implementation of a not normal list container
 *
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
   *
   */
  void pushBack(typename BaseList<ValueType>::const_reference element);

  /**
   * inserts an element at the front
   * @param element the new element to insert
   *
   */
  void pushFront(typename BaseList<ValueType>::const_reference element);

  /**
   * inserts an element at the given index
   * NOTE that the index MUST be smaller than or equal to the size of the list
   * @param index the index at which to insert the element
   * @param element the element to insert
   *
   */
  void insert(typename BaseList<ValueType>::size_type index, typename BaseList<ValueType>::const_reference element);

  /**
   * Removes the first element and adds it to the end.
   *
   */
  void rotateBack();

  /**
   * Removes the last element and adds it to the front.
   *
   */
  void rotateFront();

};


#include "List.tcpp"

#endif // LIST_H___


