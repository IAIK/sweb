/*
 * GeneralCache.cpp
 *
 *  Created on: 03.06.2012
 *      Author: chris
 */

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <map>
#include "assert.h"
#else
#include "Scheduler.h"
#include "ustl/umap.h"
#include "kprintf.h"
#endif

#include "GeneralCache.h"

namespace Cache
{

GeneralCache::GeneralCache(DeviceAdapter* cache_device,
		num_items_t soft_limit, num_items_t hard_limit) : cache_device_(cache_device),
		cache_read_strategy_(NULL),
		cache_write_strategy_(NULL),
		soft_limit_(soft_limit), hard_limit_(hard_limit),
		non_blocking_cache_(false)
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
		,map_lock_("GeneralCache")
#endif
#ifndef NO_USE_OF_MULTITHREADING
    ,item_locks_mutex_("item_locks_ vector mutex")
#endif
{
  // cache stores at least 3 items, otherwise the overhead would be too
  // big in order to make sense
  if(hard_limit_ < 3)
  {
    hard_limit_ = 3;
  }
}

GeneralCache::~GeneralCache()
{
  // cache flush
  flush();

	/*if(cache_device_ != NULL)
	{
		delete cache_device_;
	}*/

	if(cache_read_strategy_ != NULL)
	{
		delete cache_read_strategy_;
	}

	if(cache_write_strategy_ != NULL)
	{
		delete cache_write_strategy_;
	}
}

void GeneralCache::setReadStrategy(CacheReadStrategy* cache_read_strategy)
{
  cache_read_strategy_ = cache_read_strategy;
}

void GeneralCache::setWriteStrategy(CacheWriteStrategy* cache_write_strategy)
{
  cache_write_strategy_ = cache_write_strategy;
}

void GeneralCache::setCacheDevice(DeviceAdapter* cache_device)
{
  cache_device_ = cache_device;

  if(cache_write_strategy_ != NULL)
    cache_write_strategy_->setCacheDevice(cache_device);
}

Item* GeneralCache::getItem(const ItemIdentity& ident)
{
  // function is senseless without a Read-strategy
  if(cache_read_strategy_ == NULL)
    return NULL;

  // lock the given ident-slot in order to make this operation exclusive for the
  // current slot
  lockItem(ident);

  // cache item is already marked to be deleted, no more references allowed
  if(getIdentDeleteAfterReleaseState(ident))
  {
    unlockItem(ident);
    return NULL;
  }

  stats_.num_requests++;

  // try to get the Item from the Read-Cache
  Item* data = cache_read_strategy_->get(ident);

	// requested item was not in the Cache, so load it!
	if(data == NULL)
	{
	  stats_.num_misses++;
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  debug(CACHE, "getItem - cache miss, load from device!\n");
#endif

	  if(cache_device_ == NULL)
	  {
	    // no device here to read the data from...
	    unlockItem(ident);
	    return NULL;
	  }

	  // read the data from the Device
		data = cache_device_->read(ident);

		// fatal error on reading...
		if(data == NULL)
		{
		  unlockItem(ident);
		  return NULL;
		}

		// add loaded element to cache
		addItem(ident, data);
	}
	else
	{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
	  stats_.num_cache_hits++;
	  debug(CACHE, "getItem - cache hit!\n");
#endif
	}

	// avoid freeing the cache data while it is borrowed to the
	// Requester:
	incrRefCount(ident);
	unlockItem(ident);

	return data;
}

void GeneralCache::releaseItem(const ItemIdentity& ident)
{
  // lock the given ident-slot in order to make this operation exclusive for the
  // current slot
  lockItem(ident);

  // decrement ref-count
  decrRefCount(ident);

  unlockItem(ident);
}

void GeneralCache::addItem(const ItemIdentity& ident, Item* item)
{
  // somehow a senseless operation if no ReadCache is here...
  if(cache_read_strategy_ == NULL)
    return;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  debug(CACHE, "addItem - CALL\n");
#endif

  // first check if there's the need to remove an item from the cache
  if(cache_read_strategy_->getNumItems()+1 > hard_limit_)
  {
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    debug(CACHE, "addItem - hard limit exceeded, going to evict an Item.\n");
#endif
    stats_.evicted_items++;

    // cache is now full, evict an (some) item(s)
    cache_read_strategy_->evictItem();
  }

  // add new element to cache
  cache_read_strategy_->add(ident, item);

  // TODO-DEBUG: cache is NOT allowed to become full at any time!
  assert(cache_read_strategy_->getNumItems() <= hard_limit_);
}

bool GeneralCache::writeItem(const ItemIdentity& ident, Item* item, bool delete_item)
{
  // no Item object, just the Ident passed, fetch it from the read-Cache
  if(item == NULL)
  {
    if(cache_read_strategy_ == NULL)
      return false;

    // getting from the Read-Strategy
    item = cache_read_strategy_->get(ident);
    delete_item = false; // do not delete item, management is done by Read-Cache
  }

  // no write-strategy, so act like a write-trough cache
  if(cache_write_strategy_ == NULL)
  {
    if(cache_device_ != NULL)
    {
      // write the item to the underlining device
      bool ret_val = cache_device_->write(ident, item);

      // delete passed item
      if(delete_item)
      {
        delete item;
      }

      return ret_val;
    }

    return false;
  }

  // NOTE: due to the use of pointers there is no need here to
  // update the element in the read-cache!

  // delegate the write call to the Write-cache:
  return cache_write_strategy_->queueWrite(ident, item, delete_item);
}

bool GeneralCache::isItemFree(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock used_items_lock(map_lock_);
#endif

  // lock the given ident-slot in order to make this operation exclusive for the
  // current slot
  //lockItem(ident);
  if(!lockItemNonBlocking(ident))
  {
    return false;
  }

  bool item_free = true;

  for(lock_list_iterator it = borrowed_items_.begin(); it != borrowed_items_.end(); it++)
  {
    if((*(*it).first) == ident)
    {
      if(it->second > 0)
        item_free = false;
      else if(it->second == 0)
        item_free = true;
      break;
    }
  }

  unlockItem(ident);

	return item_free;
}

void GeneralCache::removeItem(const ItemIdentity& ident)
{
  if(cache_read_strategy_ == NULL)
    return;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  debug(CACHE, "removeItem - CALL\n");
  MutexLock used_items_lock(map_lock_);
#endif

  // atomic operation to the item
  lockItem(ident);

  // item removable? - all references cleared?
  bool remove_item = true;

  for(lock_list_iterator it = borrowed_items_.begin(); it != borrowed_items_.end(); it++)
  {
    if((*(*it).first) == ident)
    {
      if(it->second > 0)
      {
        // someone is still holding references to the item, remove it
        // after the last one has released it's reference
        (*(*it).first).setDeleteAfterRelease();
        remove_item = false;
      }
      else if(it->second == 0)
      {
        // perfect, no more references, delete right now
        remove_item = true;
      }
      break;
    }
  }

  // nobody holds any reference to the given Item, so it can be deleted right now
  if(remove_item)
  {
    deleteItem(ident);
  }

  unlockItem(ident);
}

void GeneralCache::deleteItem(const ItemIdentity& ident)
{
  // obtaining the Item that fits to the Ident object from the Read-Cache
  Item* item = NULL;
  if(cache_read_strategy_ != NULL) item = cache_read_strategy_->get(ident);

  // it is absolutely necessary to keep the following order for consistency
  // reasons

  // step 1: cancel all possible queued write operations of the item
  if(cache_write_strategy_ != NULL) cache_write_strategy_->removeItem(ident);

  // remove the item from the underling device physically (Item instance
  // still stays valid and it is very likely that it will be needed for
  // cleaning up the item)
  if(cache_device_ != NULL) cache_device_->remove(ident, item);

  // at least remove the Item from the Read-Strategy so that the instance
  // will be destroyed and nothing of the Item remains (neither in memory
  // nor on the device)
  if(cache_read_strategy_ != NULL) cache_read_strategy_->removeItem(ident);
}

bool GeneralCache::getIdentDeleteAfterReleaseState(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock used_items_lock(map_lock_);
#endif

  for(lock_list_iterator it = borrowed_items_.begin(); it != borrowed_items_.end(); it++)
  {
    if((*(*it).first) == ident)
    {
      return (*it).first->deleteAfterRelease();
    }
  }
  return false;
}

void GeneralCache::flush(void)
{
  if(cache_write_strategy_ == NULL)
    return;

  // do a cache flush
  cache_write_strategy_->flush();
}

bool GeneralCache::writeItemImmediately(const ItemIdentity& ident/*, Item* item*/)
{
  if(cache_write_strategy_ != NULL)
  {
    return cache_write_strategy_->enforceWrite(ident/*, item*/);
  }

  return false;

  /*// there is no write strategy, so the cache acts like a write-trough cache
  // all write-calls were already executed, nothing to flush ...
  if(cache_write_strategy_ == NULL)
  {
    return;
  }

  bool item_manually_loaded = false;

  if(item == NULL)
  {
    item = getItem(ident);
    item_manually_loaded = true;

    // item is still NULL, could not be loaded, FAIL
    return;
  }

  cache_write_strategy_->enforceWrite(ident, item);

  if(item_manually_loaded)
  {
    // decrement ref-count
    releaseItem(ident);
  }*/
}

void GeneralCache::incrRefCount(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock used_items_lock(map_lock_);
#endif

  for(lock_list_iterator it = borrowed_items_.begin(); it != borrowed_items_.end(); it++)
  {
    if((*(*it).first) == ident)
    {
      // increment reference count:
      (*it).second++;
      return;
    }
  }

  // this is the first reference to an Item with this identity,
  // so insert a new pair
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  borrowed_items_.insert(ustl::make_pair<ItemIdentity*, uint32>(ident.clone(), 1));
#else
  borrowed_items_.insert(std::make_pair<ItemIdentity*, uint32>(ident.clone(), 1));
#endif
}

void GeneralCache::decrRefCount(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock used_items_lock(map_lock_);
#endif

  for(lock_list_iterator it = borrowed_items_.begin(); it != borrowed_items_.end(); it++)
  {
    if((*(*it).first) == ident)
    {
      // increment reference count:
      (*it).second--;

      if((*it).second == 0)
      {
        if((*it).first->deleteAfterRelease())
        {
          deleteItem(*(*it).first);
        }
        // remove item from map
        delete (*it).first; // free Ident-object
        borrowed_items_.erase(it);
      }

      return;
    }
  }

  // item was not found -> indicates a fatal locking fault!
  assert(false);
}

bool GeneralCache::lockItem(const ItemIdentity& ident)
{
#ifndef NO_USE_OF_MULTITHREADING
  if(non_blocking_cache_)
  {
    return lockItemNonBlocking(ident);
  }
  lockItemBlocking(ident);
#endif // NO_USE_OF_MULTITHREADING
  return true;
}

void GeneralCache::lockItemBlocking(const ItemIdentity& ident)
{
#ifndef NO_USE_OF_MULTITHREADING
  while(true)
  {
    item_locks_mutex_.acquire("GeneralCache::lockItemBlocking");

    // check data-structure
    if(!isInVectorUnprotected(ident))
    {
      item_locks_.push_back(ident.clone());
      item_locks_mutex_.release("GeneralCache::lockItemBlocking");
      return;
    }

    item_locks_mutex_.release("GeneralCache::lockItemBlocking");
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    Scheduler::instance()->yield();
#endif
  }
#endif // NO_USE_OF_MULTITHREADING
}

bool GeneralCache::lockItemNonBlocking(const ItemIdentity& ident)
{
#ifndef NO_USE_OF_MULTITHREADING
  if(!item_locks_mutex_.acquireNonBlocking("GeneralCache::lockItemNonBlocking"))
  {
    return false;
  }

  if(isInVectorUnprotected(ident))
  {
    // no mutual exclusion today!
    item_locks_mutex_.release("GeneralCache::lockItemNonBlocking");
    return false;
  }

  item_locks_.push_back(ident.clone());
  item_locks_mutex_.release("GeneralCache::lockItemNonBlocking");
#endif // NO_USE_OF_MULTITHREADING

  // by default locking can be established (in order to work if the macro is
  // enabled
  return true;
}

bool GeneralCache::unlockItem(const ItemIdentity& ident)
{
#ifndef NO_USE_OF_MULTITHREADING
  if(non_blocking_cache_)
  {
    if(!item_locks_mutex_.acquireNonBlocking("GeneralCache::unlockItem"))
    {
      return false;
    }
  }
  else
  {
    item_locks_mutex_.acquire("GeneralCache::unlockItem");
  }

  bool item_to_realease_was_found = false;

  for(uint32 i = 0; i < item_locks_.size(); i++)
  {
    if((*item_locks_[i]) == ident)
    {
      delete item_locks_[i];
      item_locks_.erase(item_locks_.begin() + i);

      item_to_realease_was_found = true;
      break;
    }
  }

  // indicates a bad, bad, bad locking fault!!!
  assert(item_to_realease_was_found);

  item_locks_mutex_.release("GeneralCache::unlockItem");
  //
#endif // NO_USE_OF_MULTITHREADING
  return true;
}

#ifndef NO_USE_OF_MULTITHREADING
bool GeneralCache::isInVectorUnprotected(const ItemIdentity& ident)
{
  for(uint32 i = 0; i < item_locks_.size(); i++)
  {
    if((*item_locks_[i]) == ident)
    {
      return true;
    }
  }

  return false;
}
#endif // NO_USE_OF_MULTITHREADING

void GeneralCache::getStats(CacheStat& stats) const
{
  stats = stats_;
}

} // end of namespace "Cache"
