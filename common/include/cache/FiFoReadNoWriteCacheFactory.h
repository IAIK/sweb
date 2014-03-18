/**
 * Filename: FiFoReadNoWriteCacheFactory.h
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#ifndef FIFOREADNOWRITECACHEFACTORY_H_
#define FIFOREADNOWRITECACHEFACTORY_H_

#include "CacheFactory.h"

namespace Cache
{

class FiFoReadNoWriteCacheFactory : public CacheFactory
{
public:
  FiFoReadNoWriteCacheFactory();
  virtual ~FiFoReadNoWriteCacheFactory();

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

} // end of namespace "Cache"

#endif /* FIFOREADNOWRITECACHEFACTORY_H_ */
