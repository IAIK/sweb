// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Chen Qiang
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

#ifndef PointList_h___
#define PointList_h___

#include "types.h"

#ifdef STANDALONE
#include <cassert>
#else
#include "assert.h"
#include "console/kprintf.h"
#endif

//-----------------------------------------------------------------------------
/**
 * PointListElement
 *
 * used to keep track of one item on a list.
 */
template <class ContentType> class PointListElement
{
protected:
  ContentType *item_;
  PointListElement *prev_;
  PointListElement *next_;

public:
  PointListElement(ContentType *item)
  { item_ = item; prev_ = this; next_ = this; }
  virtual ~PointListElement() {}

  virtual void setNext(PointListElement<ContentType> *next) { next_ = next; }
  virtual void setPrev(PointListElement<ContentType> *prev) { prev_ = prev; }
  virtual PointListElement* getNext() { return next_; }
  virtual PointListElement* getPrev() { return prev_; }
  virtual ContentType* getItem() { return item_; }
};

//-----------------------------------------------------------------------------
/**
 * PointList
 *
 * a doubly linked list of the list elements, each of which points to a single
 * item on the list.
 */
template <class ContentType> class PointList
{
private:
  const PointList<ContentType>& operator = (const PointList<ContentType>&)
  { return(*this); }

protected:
  PointListElement<ContentType> *first_;
  PointListElement<ContentType> *last_;
  uint32 length_;

public:

  PointList() { // kprintfd("*****contructor of the PointList\n"); 
    first_ = last_ = 0; length_ = 0; }

  virtual ~PointList();

protected:
  virtual void listElementInsert(
                 PointListElement<ContentType> *new_list_element,
                 PointListElement<ContentType> *prev, 
                 PointListElement<ContentType> *next);

  /**
   * return a ListElement if that the entry contained.
   */
  virtual PointListElement<ContentType>* find(ContentType *entry);

public:

  /**
   * check the existance of the input entry
   */
  virtual bool included(ContentType *entry);

  /**
   * Insert a new entry at the end of the list.
   */
  virtual void pushBack(ContentType *new_entry);

  /**
   * Insert a new entry after the specified entry..
   */
  virtual void append(ContentType *new_entry, ContentType *entry);

  /**
   * Insert a new entry at the first of the list.
   */
  virtual void pushFront(ContentType *new_entry);

  /**
   * Insert a new entry before the specified entry..
   */
  virtual void prepend(ContentType *new_entry, ContentType *entry);

  /**
   * remove the element that the entry contained.
   */
  virtual int32 remove(ContentType *entry);
  
  /**
   * remove the element that the entry contained.
   */
  // virtual int32 remove(uint32 index);

  /**
   * remove the first element of the list and return the corresponde item
   */
  virtual ContentType* popFront();
  
  /**
   * return the point of ContetnType with the index in the list.
   */
  virtual ContentType* at(uint32 index);
  
  virtual ContentType* operator[](uint32 index);

  virtual bool empty();

  virtual uint32 getLength() { return length_; }
};

//-----------------------------------------------------------------------------
template <class ContentType>
PointList<ContentType>::~PointList()
{
  while(first_ != 0)
    popFront();
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::listElementInsert(
               PointListElement<ContentType> *new_list_element,
               PointListElement<ContentType> *prev, 
               PointListElement<ContentType> *next)
{
  prev->setNext(new_list_element);
  new_list_element->setPrev(prev);
  new_list_element->setNext(next);
  next->setPrev(new_list_element);
}

//-----------------------------------------------------------------------------
template <class ContentType>
PointListElement<ContentType>* PointList<ContentType>::find(ContentType *entry)
{
  if(first_ == 0)
    return 0;

  PointListElement<ContentType> *at = first_;
  PointListElement<ContentType> *prev;

  do
  {
    if(at->getItem() == entry)
      return at;
    prev = at;
    at = prev->getNext();
  }
  while(prev != last_);

  return 0;
}

//-----------------------------------------------------------------------------
template <class ContentType>
bool PointList<ContentType>::included(ContentType *entry)
{
  if(first_ == 0)
    return false;
  
  PointListElement<ContentType> *at = first_;
  PointListElement<ContentType> *prev;
  
  do
  {
    if(at->getItem() == entry)
      return true;
    prev = at;
    at = prev->getNext();
  } while(prev != last_);
  
  return false;
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::pushBack(ContentType *new_entry)
{
//  kprintfd("List test1\n");
  length_++;
  PointListElement<ContentType> *new_element =
                            new PointListElement<ContentType>(new_entry);
//  kprintfd("List test2\n");
  if(first_ == 0) // if the list is empty
  {
//  kprintfd("List test3\n");
    first_ = new_element;
    last_ = new_element;
//  kprintfd("List test4\n");
  }
  else
  {
//  kprintfd("List test5\n");
    PointListElement<ContentType> *at = last_;

//  kprintfd("List test6\n");
    at->setNext(new_element);
    new_element->setPrev(at);
//  kprintfd("List test7\n");

    last_ = new_element;
//  kprintfd("List test8\n");
  }
//  kprintfd("List test9\n");
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::append(ContentType *new_entry, ContentType *entry)
{
  length_++;
  PointListElement<ContentType> *new_element =
                            new PointListElement<ContentType>(new_entry);
  PointListElement<ContentType> *at;
  at = find(entry);
  assert(at != 0);
  if(at != 0)
    listElementInsert(new_element, at, at->getNext());
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::pushFront(ContentType *new_entry)
{
  length_++;
  PointListElement<ContentType> *new_element =
                              new PointListElement<ContentType>(new_entry);
  if(first_ == 0) // if the list is empty
  {
    first_ = new_element;
    last_ = new_element;
  }
  else
  {
    PointListElement<ContentType> *at = first_;

    new_element->setNext(at);
    at->setPrev(new_element);

    first_ = new_element;
  }
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::prepend(ContentType *new_entry, ContentType *entry)
{
  length_++;
  PointListElement<ContentType> *new_element =
                            new PointListElement<ContentType>(new_entry);
  PointListElement<ContentType> *at;
  at = find(entry);
  assert(at != 0);
  if(at != 0)
    listElementInsert(new_element, at->getPrev(), at);
}

//-----------------------------------------------------------------------------
template <class ContentType>
ContentType* PointList<ContentType>::popFront()
{
  assert(empty() != true);

  length_--;
  PointListElement<ContentType> *element = first_;
  ContentType *item = first_->getItem();
  if(first_ == last_)
  { // List has only one entry, now has none
    first_ = 0;
    last_ = 0;
  }
  else
    first_ = element->getNext();

  delete element;
  return item;
}

//-----------------------------------------------------------------------------
template <class ContentType>
int32 PointList<ContentType>::remove(ContentType *entry)
{
  if(first_ == 0)
    return -1;

  PointListElement<ContentType> *element = find(entry);
  if(element == 0)
    return -1;
  else
  {
    length_--;
    PointListElement<ContentType> *prev = element->getPrev();
    PointListElement<ContentType> *next = element->getNext();
    if(element == first_)
    {
      next->setPrev(next);
      first_ = next;
    }
    else if(element == last_)
    {
      prev->setNext(prev);
      last_ = prev;
    }
    else
    {
      prev->setNext(next);
      next->setPrev(prev);
    }
    delete element;
  }
  return 0;
}

//-----------------------------------------------------------------------------
/*
template <class ContentType>
int32 PointList<ContentType>::remove(uint32 index)
{
  PointListElement<ContentType> *element = this->at(index); 

  if(element == 0)
    return -1;
  else
  {
    length_--;
    PointListElement<ContentType> *prev = element->getPrev();
    PointListElement<ContentType> *next = element->getNext();
    if(element == first_)
    {
      next->setPrev(next);
      first_ = next;
    }
    else if(element == last_)
    {
      prev->setNext(prev);
      last_ = prev;
    }
    else
    {
      prev->setNext(next);
      next->setPrev(prev);
    }
    delete element;
  }
  return 0;
}
*/
//-----------------------------------------------------------------------------
template <class ContentType>
bool PointList<ContentType>::empty()
{
  kprintfd("empty test1\n");
  if(first_ == 0)
  {
  kprintfd("empty test2\n");
    return true;
  }
  else
  {
  kprintfd("empty test3\n");
    return false;
  }
}

//-----------------------------------------------------------------------------
template <class ContentType>
ContentType* PointList<ContentType>::at(uint32 index)
{
  if(index >= length_)
    return 0;
  
  PointListElement<ContentType> *prev = first_;
  PointListElement<ContentType> *at = prev;
  while(index != 0)
  {
    at = prev->getNext();
    prev = at;
    index--;
  }
  
  ContentType *item = at->getItem();
  return item;
}

//-----------------------------------------------------------------------------
template <class ContentType>
ContentType* PointList<ContentType>::operator[](uint32 index)
{
  if(index >= length_)
    return 0;
  
  PointListElement<ContentType> *prev = first_;
  PointListElement<ContentType> *at = prev;
  while(index != 0)
  {
    at = prev->getNext();
    prev = at;
    index--;
  }
  
  ContentType *item = at->getItem();
  return item;
}

#endif // PointList_h___


