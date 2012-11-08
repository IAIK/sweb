/**
 * Filename: DeviceCache.cpp
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#include "fs/DeviceCache.h"

#ifndef NULL
#define NULL 0
#endif

SectorCacheIdent::SectorCacheIdent(sector_addr_t sector_no,
    sector_addr_t block_size) : sector_no_(sector_no), block_size_(block_size)
{
}

SectorCacheIdent::~SectorCacheIdent()
{
}

bool SectorCacheIdent::operator==(const Cache::ItemIdentity& cmp)
{
  // try to cast the Interface into a SectorIdent type, if it fails
  // we know that they are not equal!
  const SectorCacheIdent* p_cmp = static_cast<const SectorCacheIdent*>(&cmp);

  //const SectorCacheIdent* p_cmp = dynamic_cast<const SectorCacheIdent*>(&cmp);

  if(p_cmp == NULL)
  {
    // the given Ident object is not the same type as this
    return false;
  }

  return (p_cmp->getSectorNumber() == this->getSectorNumber());
}

Cache::ItemIdentity* SectorCacheIdent::clone(void) const
{
  return new SectorCacheIdent(getSectorNumber(), getSectorSize());
}

sector_addr_t SectorCacheIdent::getSectorNumber(void) const
{
  return sector_no_;
}

sector_len_t SectorCacheIdent::getSectorSize(void) const
{
  return block_size_;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

SectorCacheItem::SectorCacheItem(char* sector_data) : Item(),
    sector_(sector_data)
{
}

SectorCacheItem::~SectorCacheItem()
{
  if(sector_ != NULL)
    delete[] sector_;
}

void* SectorCacheItem::getData(void)
{
  return sector_;
}

const void* SectorCacheItem::getData(void) const
{
  return sector_;
}
