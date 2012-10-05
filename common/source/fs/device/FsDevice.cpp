/**
 * Filename: FsDevice.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#include "fs/device/FsDevice.h"
#include "fs/DeviceCache.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include "debug_print.h"
#else
#include "kprintf.h"
#endif

FsDevice::FsDevice()
{
}

FsDevice::~FsDevice()
{
}

Cache::Item* FsDevice::read(const Cache::ItemIdentity& ident)
{
  debug(FS_DEVICE, "read - CALL (ident addr=%x)\n", &ident);

  const SectorCacheIdent* sector_ident = reinterpret_cast<const SectorCacheIdent*>(&ident);

  // invalid / not usable ident-object
  if(sector_ident == NULL)
  {
    debug(FS_DEVICE, "read - ERROR invalid argument.\n");
    return NULL;
  }

  sector_addr_t sector = sector_ident->getSectorNumber();
  sector_len_t block_size = sector_ident->getSectorSize();

  debug(FS_DEVICE, "read - reading from sector=%x (BlockSize=%d)\n", sector, block_size);

  // reading the Sector from the FsDevice
  char* data = new char[block_size];
  if(!readSector(sector, data, block_size))
  {
    // I/O read-error
    return NULL;
  }

  return new SectorCacheItem( data );
}

bool FsDevice::write(const Cache::ItemIdentity& ident, Cache::Item* data)
{
  debug(FS_DEVICE, "write - CALL (ident addr=%x) (data addr=%x)\n", &ident, data);

  const SectorCacheIdent* sector_ident = reinterpret_cast<const SectorCacheIdent*>(&ident);

  // error wrong ItemIdent object
  if(sector_ident == NULL || data == NULL)
  {
    debug(FS_DEVICE, "write - ERROR invalid argument(s).\n");
    return false;
  }

  debug(FS_DEVICE, "write - writing to sector=%x.\n", sector_ident->getSectorNumber());
  debug(FS_DEVICE, "write - data to write=%x.\n", data->getData());

  return writeSector(sector_ident->getSectorNumber(),
                     reinterpret_cast<const char*>(data->getData()),
                     sector_ident->getSectorSize());
}

bool FsDevice::remove(const Cache::ItemIdentity& ident __attribute__((unused)),
                      Cache::Item* item __attribute__((unused)))
{
  debug(FS_DEVICE, "remove - CALL - method is empty!\n");

  // physically not possible to remove a sector from a disk
  return false;
}
