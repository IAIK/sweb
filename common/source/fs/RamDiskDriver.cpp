#include "RamDiskDriver.h"
#include "debug.h"
#include "assert.h"
#include "BDRequest.h"
#include "kstring.h"

RamDiskDriver::RamDiskDriver(void* start_vaddr, size_t size) :
  start_vaddr_(start_vaddr),
  size_(size)
{
  debug(RAMDISK, "Create ramdisk, start: %p, size: %zx\n", start_vaddr_, size_);
  assert((((size_t)-1) - size_) >= (size_t)start_vaddr_);
}

RamDiskDriver::~RamDiskDriver()
{
  debug(RAMDISK, "Destruct ramdisk, start: %p, size: %zx\n", start_vaddr_, size_);
}

uint32 RamDiskDriver::addRequest(BDRequest * request)
{
  switch (request->getCmd())
  {
  case BDRequest::BD_CMD::BD_GET_BLK_SIZE:
    request->setResult(getSectorSize());
    request->setStatus(BDRequest::BD_RESULT::BD_DONE);
    break;
  case BDRequest::BD_CMD::BD_GET_NUM_BLOCKS:
    request->setResult(getNumSectors());
    request->setStatus(BDRequest::BD_RESULT::BD_DONE);
    break;
  case BDRequest::BD_CMD::BD_READ:
    readSector(request->getStartBlock(), request->getNumBlocks(), request->getBuffer());
    break;
  case BDRequest::BD_CMD::BD_WRITE:
    writeSector(request->getStartBlock(), request->getNumBlocks(), request->getBuffer());
    // fall-through
  default:
    request->setStatus(BDRequest::BD_RESULT::BD_DONE);
    request->setResult(0);
    break;
  }
  return 0;
}

uint32 RamDiskDriver::getNumSectors()
{
  debug(RAMDISK, "ramdisk: getNumSectors\n");
  return size_;
}

uint32 RamDiskDriver::getSectorSize()
{
  debug(RAMDISK, "ramdisk: getSectorSize\n");
  return 1;
}

int32 RamDiskDriver::readSector( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  debug(RAMDISK, "ramdisk: readSector, start: %x, num: %x, buf: %p => reading[%p, %p)\n", start_sector, num_sectors, buffer, (char*)start_vaddr_ + start_sector, (char*)start_vaddr_ + start_sector + num_sectors);
  assert(num_sectors <= getNumSectors());
  memcpy(buffer, (char*)start_vaddr_ + start_sector, num_sectors);
  return 0;
}

int32 RamDiskDriver::writeSector( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  debug(RAMDISK, "ramdisk: writeSector, start: %x, num: %x, buf: %p\n", start_sector, num_sectors, buffer);
  assert(num_sectors <= getNumSectors());
  memcpy((char*)start_vaddr_ + start_sector, buffer, num_sectors);
  return 0;
}

void RamDiskDriver::serviceIRQ()
{
  assert(false && "RamDiskDriver doesn't need a service irq");
}
