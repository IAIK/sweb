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

#ifndef SORTED_LIST_H___
#define SORTED_LIST_H___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "BaseList.h"
#include "ListNode.h"

/**
 * class List provides a basic implementation of a not sorted list container
 *
 */
template<typename ValueType>
class SortedList:public BaseList<ValueType>

{
 public:
   /**
   * default constructor for class List
   * the node size will be set to a default value
   */
  SortedList();
  /**
   * destructor for class List
   */
  virtual ~SortedList();

  /**
   * inserts an element
   * @param element the element to insert
   * @return false if the element allready exsisted and therefore not has been inserted again
   *
   */
  bool insert(typename BaseList<ValueType>::const_reference element);

  /**
   * Removes an element by value
   * @param element the value of the element to remove
   *
   */
  void remove(typename BaseList<ValueType>::value_type value);

  /**
   * Finds an element by value
   * @param element the value of the element to find
   * @return the index of the element or the position after the last element if not found
   * @pre the '<' and '==' operator has to be implemented for ValueType
   *
   */
  uint32 find(typename BaseList<ValueType>::value_type value);

private:
  /**
   * Finds the element by value or the position where it should be :)
   * @param element the value of the element to find
   * @param value (optional) the pointer to the found node will be stored in the given node pointer. 0 if not found.
   * @return the index of the node or before where it should be inserted.
   * @pre the '<' and '==' operator has to be implemented for ValueType
   *
   */
  uint32 quickfind(typename BaseList<ValueType>::value_type value,
    ListNode<ValueType> **ret_node);

};


#include "SortedList.tcpp"

#endif // SORTED_LIST_H___


