/**
 * Filename: CacheItem.h
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#ifndef CACHEITEM_H_
#define CACHEITEM_H_

namespace Cache
{

/**
 * @class interface is used to compare two concrete CacheItems
 * with each other for equality
 */
class ItemIdentity
{
public:
  ItemIdentity() : delete_after_release_(false) {}
  virtual ~ItemIdentity() {}

  /**
   * Comparison operator for two CacheItems
   */
  //virtual bool equals(const ItemIdentity& cmp) = 0;
  virtual bool operator==(const ItemIdentity& cmp) = 0;

  /**
   * clones this object and returns a new instance of it
   * Located on the heap
   * @return an identical copy of this identity
   */
  virtual ItemIdentity* clone(void) const = 0;

  /**
   * should the Item be deleted from the underling device after the
   * last reference was released
   */
  bool deleteAfterRelease(void) const { return delete_after_release_; }
  void setDeleteAfterRelease(void) { delete_after_release_ = true; }

private:

  // if flag is set the Item will be deleted if the last reference was
  // released
  bool delete_after_release_;
};

/**
 * @class represents a cache item
 */
class Item
{
public:
  Item() : dirty_(false) {}
  virtual~ Item() {}

  void setDirty(void) { dirty_ = true; }
  void setClean(void) { dirty_ = false; }
  bool isDirty(void) const { return dirty_; }

  virtual void* getData(void) = 0;
  virtual const void* getData(void) const = 0;

private:
  bool dirty_; // the item's dirty flag
};

} // end of namespace "Cache"

#endif /* CACHEITEM_H_ */
