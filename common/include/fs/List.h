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

#ifndef List_h___
#define List_h___

#include "types.h"
#include "util/assert.h"
//-----------------------------------------------------------------------------
/**
 * ListElement
 *
 * used to keep track of one item on a list.
 */
template <class ContentType> class ListElement
{
protected:
  ContentType *item_;
  ListElement *prev_;
  ListElement *next_;

public:
  ListElement(ContentType *item)
  { item_ = item; prev_ = this; next_ = this; }
  virtual ~ListElement() {}

  virtual void set_next(ListElement<ContentType> *next) { next_ = next; }
  virtual void set_prev(ListElement<ContentType> *prev) { prev_ = prev; }
  virtual ListElement* get_next() { return next_; }
  virtual ListElement* get_prev() { return prev_; }
  virtual ContentType* get_item() { return item_; }
};

//-----------------------------------------------------------------------------
/**
 * List
 *
 * a doubly linked list of the list elements, each of which points to a single
 * item on the list.
 */
template <class ContentType> class List
{
private:
  const List<ContentType>& operator = (const List<ContentType>&)
  { return(*this); }

protected:
  ListElement<ContentType> *first_;
  ListElement<ContentType> *last_;

public:
  List() { first_ = last_ = 0; }

  virtual ~List();

protected:
  virtual void list_element_insert(ListElement<ContentType> *new_list_element,
               ListElement<ContentType> *prev, ListElement<ContentType> *next);


public:

  /**
   * return a ListElement if that the entry contained.
   */
  virtual ListElement<ContentType>* find(ContentType *entry);

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

  virtual bool is_empty();
};

//-----------------------------------------------------------------------------
template <class ContentType>
List<ContentType>::~List()
{
  while(first_ != 0)
    pop_first();
}

//-----------------------------------------------------------------------------
template <class ContentType>
void List<ContentType>::list_element_insert(
               ListElement<ContentType> *new_list_element,
               ListElement<ContentType> *prev, ListElement<ContentType> *next)
{
  prev->set_next(new_list_element);
  new_list_element->set_prev(prev);
  new_list_element->set_next(next);
  next->set_prev(new_list_element);
}

//-----------------------------------------------------------------------------
template <class ContentType>
ListElement<ContentType>* List<ContentType>::find(ContentType *entry)
{
  if(is_empty())
    return 0;

  ListElement<ContentType> *at = first_;
  ListElement<ContentType> *prev;

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
void List<ContentType>::push_end(ContentType *new_entry)
{
  ListElement<ContentType> *new_element =
                            new ListElement<ContentType>(new_entry);
  if(is_empty()) // if the list is empty
  {
    first_ = new_element;
    last_ = new_element;
  }
  else
  {
    ListElement<ContentType> *at = last_;

    at->set_next(new_element);
    new_element->set_prev(at);

    last_ = new_element;
  }
}

//-----------------------------------------------------------------------------
template <class ContentType>
void List<ContentType>::append(ContentType *new_entry, ContentType *entry)
{
  ListElement<ContentType> *new_element =
                            new ListElement<ContentType>(new_entry);
  ListElement<ContentType> *at;
  at = find(entry);
  assert(at != 0);
  if(at != 0)
    list_element_insert(new_element, at, at->get_next());
}

//-----------------------------------------------------------------------------
template <class ContentType>
void List<ContentType>::push_first(ContentType *new_entry)
{
    ListElement<ContentType> *new_element =
                              new ListElement<ContentType>(new_entry);
  if(is_empty()) // if the list is empty
  {
    first_ = new_element;
    last_ = new_element;
  }
  else
  {
    ListElement<ContentType> *at = first_;

    new_element->set_next(at);
    at->set_prev(new_element);

    first_ = new_element;
  }
}

//-----------------------------------------------------------------------------
template <class ContentType>
void List<ContentType>::prepend(ContentType *new_entry, ContentType *entry)
{
  ListElement<ContentType> *new_element =
                            new ListElement<ContentType>(new_entry);
  ListElement<ContentType> *at;
  at = find(entry);
  assert(at != 0);
  if(at != 0)
    list_element_insert(new_element, at->get_prev(), at);
}

//-----------------------------------------------------------------------------
template <class ContentType>
ContentType* List<ContentType>::pop_first()
{
  if(is_empty())
    return 0;

  ListElement<ContentType> *element = first_;
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
int32 List<ContentType>::remove(ContentType *entry)
{
  if(is_empty())
    return -1;

  ListElement<ContentType> *element = find(entry);
  if(element == 0)
    return -1;
  else
  {
    ListElement<ContentType> *prev = element->get_prev();
    ListElement<ContentType> *next = element->get_next();
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
bool List<ContentType>::is_empty()
{
  if(first_ == 0)
    return true;
  else
    return false;
}

//-----------------------------------------------------------------------------

#endif // List_h___


