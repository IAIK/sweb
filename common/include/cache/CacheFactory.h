/**
 * Filename: CacheFactory.h
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#ifndef CACHEFACTORY_H_
#define CACHEFACTORY_H_

#include "CacheStrategy.h"


namespace Cache
{
class GeneralCache;
class DeviceAdapter;
/**
 * @class an abstract Factory, defining a Factory interface to produce
 * different types of caches
 */
class CacheFactory
{
public:
  CacheFactory();
  virtual ~CacheFactory();

  /**
   * factory method
   * @param device the Device associated with the Cache, can be NULL if not wanted!
   * @param max_items maximal allowed items in the cache
   * @return a new instance of a GeneralCache
   */
  virtual GeneralCache* getNewCache(DeviceAdapter* device, num_items_t max_items) const;

protected:

  /**
   * creates a new CacheReadStrategy and returns it
   * @param cache the hosting Cache
   */
  virtual CacheReadStrategy* getReadStrategy(GeneralCache* cache) const = 0;

  /**
   * creates a new CacheWriteStrategy and returns an instance of it
   * @param device the Device associated with the Cache and the
   * CacheWriteStrategy
   */
  virtual CacheWriteStrategy* getWriteStrategy(DeviceAdapter* device) const = 0;

};

} // end of namespace "Cache"

#endif /* CACHEFACTORY_H_ */
