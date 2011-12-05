/**
 * @file arch_bd_virtual_device.cpp
 *
 */

#include "arch_bd_virtual_device.h"
#include "kmalloc.h"
#include "string.h"

BDVirtualDevice::BDVirtualDevice(BDDriver * driver, uint32 offset, uint32 num_sectors, uint32 sector_size, char *name, bool writable) :
Inode( 0, I_BLOCKDEVICE )
{
  offset_        = offset;
  num_sectors_   = num_sectors;
  sector_size_   = sector_size;
  block_size_    = sector_size; // should changed with setBlockSize() if needed
  writable_      = writable;
  driver_        = driver;

  debug(BD_VIRT_DEVICE, "ctor: offset = %d, num_sectors = %d,\n  sector_size = %d, "
                        "name = %s \n", offset, num_sectors, sector_size, name);

  name_         = new char[ strlen( name ) + 1 ];
  strncpy( name_, name, strlen(name) );
  name_[ strlen(name) ] = '\0';

  debug(BD_VIRT_DEVICE, "ctor:registering with DeviceFS\n");
  i_superblock_ = DeviceFSSuperBlock::getInstance();
  DeviceFSSuperBlock::getInstance()->addDevice( this, name_ );
  debug(BD_VIRT_DEVICE, "ctor:registered with DeviceFS\n");

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
   assert(offset % block_size_ == 0);
   assert(size % block_size_ == 0);
   debug(BD_VIRT_DEVICE, "readData\n");
   uint32 blocks2read = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;	

   debug(BD_VIRT_DEVICE, "blocks2read %d\n", blocks2read );
   BDRequest bd(dev_number_, BDRequest::BD_READ, blockoffset, blocks2read, buffer);
   addRequest ( &bd );

   while( bd.getStatus() == BDRequest::BD_QUEUED && jiffies++ < 5 )
     __asm__ __volatile__ ( "hlt" );

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

   while( bd.getStatus() == BDRequest::BD_QUEUED && jiffies++ < 5 )
     __asm__ __volatile__ ( "hlt" );

   if( bd.getStatus() != BDRequest::BD_DONE )
     return -1;
   else
     return size;
};
