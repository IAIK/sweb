/**
 * @file arch_bd_virtual_device.cpp
 *
 */

#include "ArchInterrupts.h"
#include "arch_bd_virtual_device.h"
#include "arch_bd_driver.h"
#include "arch_bd_request.h"
#include "kmalloc.h"
#include "string.h"
#include "debug.h"
#include "console/kprintf.h"

BDVirtualDevice::BDVirtualDevice(BDDriver * driver, uint32 offset, uint32 num_sectors, uint32 sector_size, const char *name, bool writable)
{
  offset_        = offset;
  num_sectors_   = num_sectors;
  sector_size_   = sector_size;
  block_size_    = sector_size; // should changed with setBlockSize() if needed
  writable_      = writable;
  driver_        = driver;

  partition_type_ = 0x00; // by default not set

  debug(BD_VIRT_DEVICE, "ctor: offset = %d, num_sectors = %d,\n  sector_size = %d, "
                        "name = %s \n", offset, num_sectors, sector_size, name);

  name_         = new char[ strlen( name ) + 1 ];
  strncpy( name_, name, strlen(name) );
  name_[ strlen(name) ] = '\0';
  dev_number_   = 0;
};


void BDVirtualDevice::addRequest(BDRequest * command) 
{
  command->setResult( 5 );
  switch( command->getCmd() )
  {
    case BDRequest::BD_GET_BLK_SIZE:
      command->setResult( block_size_ );
      command->setStatus( BDRequest::BD_DONE );
      break;
    case BDRequest::BD_GET_NUM_BLOCKS: 
      command->setResult( getNumBlocks() );
      command->setStatus( BDRequest::BD_DONE );
      break;
    case BDRequest::BD_READ:
    case BDRequest::BD_WRITE:
      //start block and num blocks will be interpreted as start sector and num sectors
      command->setStartBlock( command->getStartBlock() * (block_size_ / sector_size_) + offset_ );
      command->setNumBlocks( command->getNumBlocks() * (block_size_ / sector_size_));
    default:
      command->setResult( driver_->addRequest( command ));
      break;
  }
  return;
};


int32 BDVirtualDevice::readData(uint32 offset, uint32 size, char *buffer)
{
   assert(buffer);
   assert(offset % block_size_ == 0);
   assert(size % block_size_ == 0);
   debug(BD_VIRT_DEVICE, "readData\n");
   uint32 blocks2read = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;	

   debug(BD_VIRT_DEVICE, "blocks2read %d\n", blocks2read );
   BDRequest bd(dev_number_, BDRequest::BD_READ, blockoffset, blocks2read, buffer);
   addRequest ( &bd );

   if( bd.getStatus() != BDRequest::BD_DONE )
   {
     return -1;
   }
   return size;
};

int32 BDVirtualDevice::writeData(uint32 offset, uint32 size, char *buffer)
{
   assert(offset % block_size_ == 0);
   assert(size % block_size_ == 0);
   debug(BD_VIRT_DEVICE, "writeData\n");
   uint32 blocks2write = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;

   BDRequest bd(dev_number_ ,BDRequest::BD_WRITE, blockoffset, blocks2write, buffer);
   addRequest ( &bd );

   bool interrupt_context = ArchInterrupts::disableInterrupts();
   ArchInterrupts::enableInterrupts();

   while( bd.getStatus() == BDRequest::BD_QUEUED && jiffies++ < IO_TIMEOUT )
     ArchInterrupts::yieldIfIFSet();

   if( !interrupt_context )
     ArchInterrupts::disableInterrupts();

   if( bd.getStatus() != BDRequest::BD_DONE )
     return -1;
   else
     return size;
};

void BDVirtualDevice::setPartitionType(uint8 part_type)
{
  partition_type_ = part_type;
}

uint8 BDVirtualDevice::getPartitionType(void) const
{
  return partition_type_;
}
