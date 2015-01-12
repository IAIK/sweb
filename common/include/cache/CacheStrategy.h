/**
 * Filename: CacheStrategy.h
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#ifndef CACHESTRATEGY_H_
#define CACHESTRATEGY_H_

namespace Cache
{

#ifdef _SWEB
typedef uint32 num_items_t;
#else
typedef unsigned int num_items_t;
#endif

// forwards
class Item;
class ItemIdentity;
class GeneralCache;
class DeviceAdapter;

/**
 * @class a Cache reading strategy
 */
class CacheReadStrategy
{
public:
  CacheReadStrategy(GeneralCache* cache) : cache_(cache) {}
  virtual ~CacheReadStrategy() {}

  /**
   * searches the cache-data structure for an item with the
   * given identity and returns it's data if it was found
   * @param ident
   * @return data or NULL if not present
   */
  virtual Item* get(const ItemIdentity& ident) = 0;

  /**
   * adds a new item to the cache
   * @param ident the identity of the new item
   * @param item the item's data
   * @return true / false
   */
  virtual bool add(const ItemIdentity& ident, Item* item) = 0;

  /**
   * removes an item from the read-cache pool
   *
   * @param ident the Item with the given Identity will be removed
   */
  virtual void removeItem(const ItemIdentity& ident) = 0;

  /**
   * evicts an item according to the cache-strategy from the cache
   */
  virtual void evictItem(void) = 0;

  /**
   * clears all currently free items from the cache
   */
  virtual void clear(void) = 0;

  /**
   * returns the number of items in the cache
   * @return number of elements in the cache
   */
  virtual num_items_t getNumItems(void) const = 0;

protected:
  GeneralCache* cache_;
};

/**
 * @class a Cache writing strategy
 */
class CacheWriteStrategy
{
public:
  /**
   * CacheWriteStrategy - constructor
   * @param device associated device to write data to
   */
  CacheWriteStrategy(DeviceAdapter* device) : device_(device) {}
  virtual ~CacheWriteStrategy() {}

  /**
   * caches the used Cache::DeviceAdapter
   *
   * @param cache_device the new DeviceAdapter to use
   */
  virtual void setCacheDevice(DeviceAdapter* cache_device) = 0;

  /**
   * queues a new write operation for the given item
   * the CacheWriteStrategy implementation will decide when the Item
   * is written out to the Device
   * @param ident the item's identity
   * @param data the item's data to be written
   * @param delete_item
   * @return true if the write operation was successfully queued
   * false if the operation could not be queued
   */
  virtual bool queueWrite(const ItemIdentity& ident, Item* item, bool delete_item) = 0;

  /**
   * If the given item is in the write queue it will be taken out of it
   * and beeing immediately written to the Device without any delay!
   * If no write-operation is queued for the given Item nothing will be done!
   * This method is required for example in so called Write-Back strategies
   *
   * @param ident
   */
  virtual bool enforceWrite(const ItemIdentity& ident) = 0;

  /**
   * flushes all pending writing operations
   */
  virtual void flush(void) = 0;

  /**
   * removes an item from the write-cache pool
   * this method will discard all queued write operations of the item
   *
   * @param ident the Item with the given Identity will be removed
   */
  virtual void removeItem(const ItemIdentity& ident) = 0;

protected:
  // the associated Device
  DeviceAdapter* device_;
};

}

#endif /* CACHESTRATEGY_H_ */
