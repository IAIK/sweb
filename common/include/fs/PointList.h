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
#include "assert.h"
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

  virtual void set_next(PointListElement<ContentType> *next) { next_ = next; }
  virtual void set_prev(PointListElement<ContentType> *prev) { prev_ = prev; }
  virtual PointListElement* get_next() { return next_; }
  virtual PointListElement* get_prev() { return prev_; }
  virtual ContentType* get_item() { return item_; }
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
  PointList() { first_ = last_ = 0; length_ = 0; }

  virtual ~PointList();

protected:
  virtual void list_element_insert(
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
  virtual bool is_included(ContentType *entry);

  /**
   * Insert a new entry at the end of the list.
   */
  virtual void push_end(ContentType *new_entry);

  /**
   * Insert a new entry after the specified entry..
   */
  virtual void append(ContentType *new_entry, ContentType *entry);

  /**
   * Insert a new entry at the first of the list.
   */
  virtual void push_first(ContentType *new_entry);

  /**
   * Insert a new entry before the specified entry..
   */
  virtual void prepend(ContentType *new_entry, ContentType *entry);

  /**
   * remove the element that the entry contained.
   */
  virtual int32 remove(ContentType *entry);

  /**
   * remove the first element of the list and return the corresponde item
   */
  virtual ContentType* pop_first();
  
  /**
   * return the point of ContetnType with the index in the list.
   */
  virtual ContentType* at(uint32 index);

  virtual bool is_empty();

  virtual uint32 getLength() { return length_; }
};

//-----------------------------------------------------------------------------
template <class ContentType>
PointList<ContentType>::~PointList()
{
  while(first_ != 0)
    pop_first();
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::list_element_insert(
               PointListElement<ContentType> *new_list_element,
               PointListElement<ContentType> *prev, 
               PointListElement<ContentType> *next)
{
  prev->set_next(new_list_element);
  new_list_element->set_prev(prev);
  new_list_element->set_next(next);
  next->set_prev(new_list_element);
}

//-----------------------------------------------------------------------------
template <class ContentType>
PointListElement<ContentType>* PointList<ContentType>::find(ContentType *entry)
{
  if(is_empty())
    return 0;

  PointListElement<ContentType> *at = first_;
  PointListElement<ContentType> *prev;

  do
  {
    if(at->get_item() == entry)
      return at;
    prev = at;
    at = prev->get_next();
  }
  while(prev != last_);

  return 0;
}

//-----------------------------------------------------------------------------
template <class ContentType>
bool PointList<ContentType>::is_included(ContentType *entry)
{
  if(is_empty())
    return false;
  
  PointListElement<ContentType> *at = first_;
  PointListElement<ContentType> *prev;
  
  do
  {
    if(at->get_item() == entry)
      return true;
    prev = at;
    at = prev->get_next();
  } while(prev != last_);
  
  return false;
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::push_end(ContentType *new_entry)
{
  length_++;
  PointListElement<ContentType> *new_element =
                            new PointListElement<ContentType>(new_entry);
  if(is_empty()) // if the list is empty
  {
    first_ = new_element;
    last_ = new_element;
  }
  else
  {
    PointListElement<ContentType> *at = last_;

    at->set_next(new_element);
    new_element->set_prev(at);

    last_ = new_element;
  }
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
    list_element_insert(new_element, at, at->get_next());
}

//-----------------------------------------------------------------------------
template <class ContentType>
void PointList<ContentType>::push_first(ContentType *new_entry)
{
  length_++;
  PointListElement<ContentType> *new_element =
                              new PointListElement<ContentType>(new_entry);
  if(is_empty()) // if the list is empty
  {
    first_ = new_element;
    last_ = new_element;
  }
  else
  {
    PointListElement<ContentType> *at = first_;

    new_element->set_next(at);
    at->set_prev(new_element);

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
    list_element_insert(new_element, at->get_prev(), at);
}

//-----------------------------------------------------------------------------
template <class ContentType>
ContentType* PointList<ContentType>::pop_first()
{
  assert(is_empty() != true)

  length_--;
  PointListElement<ContentType> *element = first_;
  ContentType *item = first_->get_item();
  if(first_ == last_)
  { // List has only one entry, now has none
    first_ = 0;
    last_ = 0;
  }
  else
    first_ = element->get_next();

  delete element;
  return item;
}

//-----------------------------------------------------------------------------
template <class ContentType>
int32 PointList<ContentType>::remove(ContentType *entry)
{
  if(is_empty())
    return -1;

  PointListElement<ContentType> *element = find(entry);
  if(element == 0)
    return -1;
  else
  {
    length_--;
    PointListElement<ContentType> *prev = element->get_prev();
    PointListElement<ContentType> *next = element->get_next();
    if(element == first_)
    {
      next->set_prev(next);
      first_ = next;
    }
    else if(element == last_)
    {
      prev->set_next(prev);
      last_ = prev;
    }
    else
    {
      prev->set_next(next);
      next->set_prev(prev);
    }
    delete element;
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class ContentType>
bool PointList<ContentType>::is_empty()
{
  if(first_ == 0)
    return true;
  else
    return false;
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
    at = prev->get_next();
    prev = at;
    index--;
  }
  
  ContentType *item = at->get_item();
  return item;
}

//-----------------------------------------------------------------------------

#endif // PointList_h___


