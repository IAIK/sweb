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

  //debug(BD_VIRT_DEVICE, "ctor:calling string functions\n");
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
      command->setResult( num_sectors_ * block_size_ / sector_size_ );
      command->setStatus( BDRequest::BD_DONE );
      break;
    case BDRequest::BD_READ: 
    case BDRequest::BD_WRITE:
      command->setStartBlock( command->getStartBlock() * block_size_ / sector_size_ + offset_ );
      command->setNumBlocks( command->getNumBlocks() * block_size_ / sector_size_);
    default:
      command->setResult( driver_->addRequest( command ));
      break;
  }
  return;
};


int32 BDVirtualDevice::readData(int32 offset, int32 size, char *buffer) 
{
   debug(BD_VIRT_DEVICE, "readData\n");
   uint32 blocks2read = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;	

   if( size%block_size_ )
     blocks2read++;

   debug(BD_VIRT_DEVICE, "blocks2read %d\n", blocks2read );
   char *my_buffer = (char *) kmalloc( blocks2read *block_size_*sizeof(char) );
   BDRequest * bd = 
   new BDRequest(0, BDRequest::BD_READ, blockoffset, blocks2read, my_buffer);
   addRequest ( bd );

   while( bd->getStatus() == BDRequest::BD_QUEUED &&
          jiffies++ < 50000 );

   if( bd->getStatus() != BDRequest::BD_DONE )
   {
     kfree( my_buffer );
     delete bd;
     return -1;
   }

   memcpy( buffer, my_buffer + (offset%block_size_), size );
   //debug(BD_VIRT_DEVICE, "memcpied\n" );
   kfree( my_buffer );
   delete bd;
   return size;
};

int32 BDVirtualDevice::writeData(int32 offset, int32 size, const char *buffer)
{
   //debug(BD_VIRT_DEVICE, "writeData\n");
   uint32 blocks2write = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;

   char *my_buffer;
   if( size%block_size_ )
   {
     blocks2write++;
     my_buffer = (char *) kmalloc( blocks2write*block_size_*sizeof(char) );

     if( readData( blockoffset, blocks2write*block_size_*sizeof(char), my_buffer) == -1 )
     {
       kfree( my_buffer );
       return -1;
     }
   }
   else
     my_buffer = (char *) kmalloc( blocks2write*block_size_*sizeof(char) );

   memcpy( my_buffer + (offset%block_size_), buffer, size );

   BDRequest * bd = 
   new BDRequest(0,BDRequest::BD_WRITE, blockoffset, blocks2write, my_buffer);
   addRequest ( bd );

   while( bd->getStatus() == BDRequest::BD_QUEUED && 
          jiffies++ < 50000 );

   BDRequest::BD_RESULT stat = bd->getStatus();

   kfree( my_buffer );
   delete bd;

   if( stat != BDRequest::BD_DONE )
     return -1;
   else
     return size;
};
