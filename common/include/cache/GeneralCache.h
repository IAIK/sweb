/*
 * GeneralCache.h
 *
 *  Created on: 03.06.2012
 *      Author: chris
 */

#ifndef GENERALCACHE_H_
#define GENERALCACHE_H_

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <map>
#include "types.h"
#else
#include "ustl/umap.h"
#include "Mutex.h"
#endif

#ifndef NULL
#define NULL 0
#endif

#include "CacheItem.h"
#include "CacheStrategy.h"

namespace Cache
{

class CacheReadStrategy;
class CacheWriteStrategy;

typedef uint32 num_items_t;

/**
 * @class adapter interface to wrap around an existing Device to
 * be handled with the general Cache
 */
class DeviceAdapter
{
public:
	DeviceAdapter() {}
	virtual ~DeviceAdapter() {}

	/**
	 * requests a Cache Object from the Device in order to add
	 * it to the Cache
	 * @param ident the Cache Item Identity required by the resource class
	 * to locate the requested item
	 * @param[out] data pointer will be filled with the read data
	 * @return the data read out from the Device
	 */
	virtual Item* read(const ItemIdentity& ident) = 0;

	/**
	 * writes data to the device
	 * @param ident the cache object identity, required by the Device to
	 * be able to write the data
	 * @param data the data to be written
	 * @return true in case of success / false otherwise
	 */
	virtual bool write(const ItemIdentity& ident, Item* data) = 0;

	/**
	 * removes the Item with the given Identity from the Device
	 *
	 * @param ident the Identity of the Item to remove
	 * @return true if the Item was successfully removed, false if not
	 */
	virtual bool remove(const ItemIdentity& ident, Item* item = NULL) = 0;
};

/**
 * @class a small struct carrying statistical informations
 * about the GeneralCache
 */
struct CacheStat
{
  uint32 num_requests;      // number of requests (getItem() calls)
  uint32 num_cache_hits;    // number of cache hits
  uint32 num_misses;        // number of cache misses
  uint32 evicted_items;     // number of evicted items
};

/**
 * @class this class provides a very general and common Cache Interface
 * usage is as follows: instead of communicating with a devices or
 * resource provider direct, just communicate with a realization of
 * this general Cache. The cache expects a reference to an adapter for
 * the device used by you.
 * This cache implementation is fully multi-threading able. It grants you that
 * a required Item will not be removed from the cache until released later on.
 * NOTE, that the cache will NOT provide locking for your data item. Mutual
 * Exclusion for the Item itself has to be assured by the cache-user!!!
 */
class GeneralCache
{
public:
	/**
	 * General Cache constructor
	 * @param cache_device optional - a Device associated with the Cache, if a
	 * Device is provided, Items will be automatically loaded from the Device on
	 * cache misses
	 *
	 * @param cache_read_strategy the reading strategy of the cache, if not provided the
	 * Cache will be treated like a Write cache, read method will always return with
	 * error codes
	 *
	 * @param cache_write_strategy the writing strategy of the cache, if not
	 * provided the Cache will be treated like a Read cache, write method will always
	 * return with error codes
	 *
	 * @param soft_limit currently unused, ignored
	 * @param hard_limit the maximal allowed elements in the cache
	 */
	GeneralCache(DeviceAdapter* cache_device,
							 num_items_t soft_limit, num_items_t hard_limit);

	/**
	 * destruction will lead to a general cache flush!
	 *
	 * deletes the Cache-Read and the Cache-Write strategy, but not the
	 * given DeviceAdapter!
	 */
	virtual ~GeneralCache();

	/**
	 * sets the Read / Write-strategy of the Cache
	 * NOTE: the GeneralCache-class will take-over control over the memory
	 * of the applied Strategies. It the destructor of GeneralCache is
	 * called it will delete all currently applied strategies
	 */
	void setReadStrategy(CacheReadStrategy* cache_read_strategy);
  void setWriteStrategy(CacheWriteStrategy* cache_write_strategy);

  /**
   * applies a new Cache Device to the GeneralCache
   * @param cache_device the new Device
   */
  void setCacheDevice(DeviceAdapter* cache_device);

	/**
	 * ReadCache: getting an object from the cache, if not present it will be
	 * read from the Device if a Device is available and the new read item
	 * will be inserted to the Read-Cache. If the item causes the Read Cache
	 * to become full at least one item will be evicted according to the
	 * used CacheReadStrategy.
	 * If the call was successful the cache-internal reference counter will
	 * be increased, so it is guaranteed that the Item won't be deleted unit
	 * it was released again. (see releaseItem() for details)
	 *
	 * @param ident the identification object for the Item
	 * @param the requested item's data or NULL if an error had happened
	 */
	virtual Item* getItem(const ItemIdentity& ident);

	/**
	 * ReadCache: frees a requested item, this indicates the cache that the
	 * item is not used anymore and can therefore be deleted if necessary
	 *
	 * @param ident the item's identity
	 */
	virtual void releaseItem(const ItemIdentity& ident);

	/**
	 * ReadCache: adds a new Item to the read-cache; if the item causes
	 * the ReadCache to become full at least one item will be evicted
	 * according to the used CacheReadStrategy.
	 *
	 * @param ident the
	 * @param item
	 */
	virtual void addItem(const ItemIdentity& ident, Item* item);

	/**
	 * WriteCache - tells the Write cache to write the Item to the Device
	 * NOTE: this does not add the Item into the *Read-Cache* (call addItem()
	 * to achieve that)
	 * In case not Write-Strategy is applied the Cache will act like a write-
	 * through cache, otherwise the time of write operation will be decided
	 * by the Write-Strategy.
	 *
	 * @param ident the identity of the Item
	 * @param item the item's data to be written, if NULL the data will be requested
	 * from the Read-Strategy
	 * @param delete_item if true and an item was passed it will be deleted after
	 * the write operation was executed
	 * @return true if the write operation was successfully executed or queued
	 * false if a occurred
	 */
	virtual bool writeItem(const ItemIdentity& ident, Item* item = NULL, bool delete_item = false);

	/**
	 * getting the lock-state of an Cache item
	 *
	 * @param ident the identity for the Item
	 * @return true if the item is currently not referenced; false otherwise
	 */
	virtual bool isItemFree(const ItemIdentity& ident);

	/**
	 * removes an Item from Cache and it's underling resource
	 *
	 * @param ident the identity of the item to remove
	 */
	virtual void removeItem(const ItemIdentity& ident);

	/**
	 * WriteCache: flushes the all pending write operations of the cache
	 */
	virtual void flush(void);

	/**
	 * writes the given item immediately to the device without any
	 * if there was a write operation for the item pending it will
	 * be removed from the queue, so that the item is just written once
	 *
	 * @param ident the ItemIdentity of the item to write
	 * @param item the item data to write back to the device (optional)
	 */
	bool writeItemImmediately(const ItemIdentity& ident);

	/**
	 * getting the current cache-statistic
	 * NOTE: this one is not locked so data might be inconsistent
	 *
	 * @param[out] stat filling struct with current statistic data
	 */
	void getStats(CacheStat& stats) const;

protected:

	// the adapted device the cache communicates with
	DeviceAdapter* cache_device_;

	// the cache-strategies
	CacheReadStrategy* cache_read_strategy_;
	CacheWriteStrategy* cache_write_strategy_;

	// the soft and the hard limit, according to the number of cache
	// elements
	num_items_t soft_limit_;
	num_items_t hard_limit_;

private:

	/**
	 * increments / decrements the reference count for the given Item-Identity
	 * @param ident
	 */
	void incrRefCount(const ItemIdentity& ident);
	void decrRefCount(const ItemIdentity& ident);

	/**
	 * removes an Item from the Cache and the underling device
	 *
	 * @param ident
	 */
	void deleteItem(const ItemIdentity& ident);

	// should the cache operate in blocking or real-time (non-blocking mode)?
	bool non_blocking_cache_;

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
	typedef std::map<ItemIdentity*, unsigned int>::iterator lock_list_iterator;
	std::map<ItemIdentity*, unsigned int> borrowed_items_;
#else
  // the borrowed data-items
  typedef ustl::map<ItemIdentity*, uint32>::iterator lock_list_iterator;
  ustl::map<ItemIdentity*, uint32> borrowed_items_;
  Mutex map_lock_;
#endif

  // the Cache's statistics
  CacheStat stats_;

  /**
   * searches the map of borrowed cache items for the ItemIdentity
   * object that compares to the given one
   * this method is used to determine whether the deleteAfterRelease
   * flag is set in the original object
   *
   * @param ident
   * @return
   */
  bool getIdentDeleteAfterReleaseState(const ItemIdentity& ident);

  /**
   * Acquires mutual exclusion for the given Item
   * blocking call, will return if mutual exclusion was successfully
   * established for the given item
   *
   * @param ident the Cache item to establish mutual exclusion for
   */
  void lockItemBlocking(const ItemIdentity& ident);

  /**
   * Acquires mutual exclusion for the given Item
   * blocking call, will return if mutual exclusion was successfully
   * established for the given item
   *
   * @param ident the Cache item to establish mutual exclusion for
   * @return true / false
   */
  bool lockItemNonBlocking(const ItemIdentity& ident);

  /**
   * locks an the item that has the given identity. It calls either
   * lockItemBlocking() or lockItemNonBlocking(). Which method is called
   * is decided by the current state of the non_blocking_cache_ flag.
   *
   * @param ident the Cache item to establish mutual exclusion for
   * @return true if the mutual exclusion could be established; false otherwise
   */
  bool lockItem(const ItemIdentity& ident);

  /**
   * unlocks a previously locked item
   *
   * @param ident
   * @return true if the item was successfully unlocked
   */
  bool unlockItem(const ItemIdentity& ident);

#ifndef NO_USE_OF_MULTITHREADING

  /**
   * searches the map for an item with the given identity and returns
   * true if the item was found in the map or false otherwise
   *
   * @param ident
   * @return true / false
   */
  bool isInVectorUnprotected(const ItemIdentity& ident);

  // a slot-specific locking-aid
  Mutex item_locks_mutex_;
  ustl::vector<ItemIdentity*> item_locks_;
#endif // NO_USE_OF_MULTITHREADING

};

} // end of namespace "Cache"

#endif /* GENERALCACHE_H_ */
