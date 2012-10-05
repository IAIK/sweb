/**
 * Filename: FifoReadWriteBackCacheFactory.h
 * Description:
 *
 * Created on: 04.09.2012
 * Author: chris
 */

#ifndef FIFOREADWRITEBACKCACHEFACTORY_H_
#define FIFOREADWRITEBACKCACHEFACTORY_H_

#include "CacheFactory.h"

namespace Cache
{

/**
 * @class FifoReadWriteBackCacheFactory creates a Read : Fifo and
 * Write : Write-Back cache
 */
class FifoReadWriteBackCacheFactory : public Cache::CacheFactory
{
public:
  FifoReadWriteBackCacheFactory();
  virtual ~FifoReadWriteBackCacheFactory();

protected:

  /**
   * creates a new CacheReadStrategy and returns it
   * @param cache the hosting Cache
   */
  virtual CacheReadStrategy* getReadStrategy(GeneralCache* cache) const;

  /**
   * creates a new CacheWriteStrategy and returns an instance of it
   * @param device the Device associated with the Cache and the
   * CacheWriteStrategy
   */
  virtual CacheWriteStrategy* getWriteStrategy(DeviceAdapter* device) const;

};

} // end of namespace "cache"

#endif /* FIFOREADWRITEBACKCACHEFACTORY_H_ */
