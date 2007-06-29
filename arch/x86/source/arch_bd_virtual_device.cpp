/********************************************************************
*
*    $Id: arch_bd_virtual_device.cpp,v 1.8 2006/10/13 11:38:12 btittelbach Exp $
*    $Log: arch_bd_virtual_device.cpp,v $
*    Revision 1.7  2005/11/25 20:16:24  woswasi
*    even in small revisions can be bugs... *grumml*
*
*    Revision 1.5  2005/11/24 23:38:35  nelles
*
*
*     Block devices fix.
*
*     Committing in .
*
*     Modified Files:
*     	arch_bd_ata_driver.cpp arch_bd_ide_driver.cpp
*     	arch_bd_manager.cpp arch_bd_virtual_device.cpp
*
*    Revision 1.4  2005/11/20 21:18:08  nelles
*
*         Committing in .
*
*          Another block device update ... Interrupts are now functional fixed some
*          8259 problems .. Reads and Writes tested  ....
*
*         Modified Files:
*     	include/arch_bd_ata_driver.h include/arch_bd_request.h
*     	include/arch_bd_virtual_device.h source/8259.cpp
*     	source/ArchInterrupts.cpp source/InterruptUtils.cpp
*     	source/arch_bd_ata_driver.cpp
*     	source/arch_bd_virtual_device.cpp source/arch_interrupts.s
*
*    Revision 1.3  2005/10/02 12:27:55  nelles
*
*     Committing in .
*
*    	DeviceFS patch. The devices can now be accessed through VFS.
*
*
*
*
*     Modified Files:
*     	Makefile arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/InterruptUtils.cpp arch/x86/source/Makefile
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     	arch/x86/source/arch_serial.cpp
*     	arch/x86/source/arch_serial_manager.cpp
*     	common/include/console/Terminal.h
*     	common/include/drivers/serial.h common/include/fs/Inode.h
*     	common/include/fs/Superblock.h common/include/fs/fs_global.h
*     	common/include/kernel/TestingThreads.h
*     	common/source/console/FrameBufferConsole.cpp
*     	common/source/console/Makefile
*     	common/source/console/Terminal.cpp
*     	common/source/console/TextConsole.cpp
*     	common/source/fs/Dentry.cpp common/source/fs/Makefile
*     	common/source/fs/PathWalker.cpp
*     	common/source/fs/Superblock.cpp
*     	common/source/fs/VfsSyscall.cpp common/source/kernel/main.cpp
*     	utils/bochs/bochsrc
*     Added Files:
*     	common/include/drivers/chardev.h
*     	common/include/fs/devicefs/DeviceFSSuperblock.h
*     	common/include/fs/devicefs/DeviceFSType.h
*     	common/source/fs/devicefs/DeviceFSSuperblock.cpp
*     	common/source/fs/devicefs/DeviceFSType.cpp
*     	common/source/fs/devicefs/Makefile
*
*    Revision 1.2  2005/09/18 20:46:52  nelles
*
*     Committing in .
*
*     Modified Files:
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_manager.h
*     	arch/x86/include/arch_bd_request.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     ----------------------------------------------------------------------
*
********************************************************************/

#include "arch_bd_virtual_device.h"
#include "kmalloc.h"
#include "string.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

BDVirtualDevice::BDVirtualDevice(BDDriver * driver, uint32 offset, uint32 num_blocks, uint32 block_size, char *name, bool writable) :
Inode( 0, I_BLOCKDEVICE )
{
    //kprintfd("BDVirtualDevice::ctor:entered");
    offset_       = offset;
    num_blocks_   = num_blocks;
    block_size_   = block_size;
    writable_     = writable;
    driver_       = driver;
    
    //kprintfd("BDVirtualDevice::ctor:calling string functions\n");
    name_         = new char[ strlen( name ) + 1 ];
    strncpy( name_, name, strlen(name) );
    name_[ strlen(name) ] = '\0';
    //kprintfd("BDVirtualDevice::ctor:string functions done\n");
    
    //kprintfd("BDVirtualDevice::ctor:registering with DeviceFS\n");
    i_superblock_ = DeviceFSSuperBlock::getInstance();
    DeviceFSSuperBlock::getInstance()->addDevice( this, name_ );
    //kprintfd("BDVirtualDevice::ctor:registered with DeviceFS\n");
    
    dev_number_   = 0;
};

    
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////    
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
      command->setResult( num_blocks_ );
      command->setStatus( BDRequest::BD_DONE );
      break;
    case BDRequest::BD_READ: 
    case BDRequest::BD_WRITE:
      command->setStartBlock( command->getStartBlock() + offset_ );
   default:
      command->setResult( driver_->addRequest( command ));
      break;
  }
  return;
};

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////   
    
int32 BDVirtualDevice::readData(int32 offset, int32 size, char *buffer) 
{
   //kprintfd("BDVirtualDevice::readData\n");
   uint32 blocks2read = size/block_size_, jiffies = 0;
   uint32 blockoffset = offset/block_size_;	
   
   if( size%block_size_ )
     blocks2read++;

   //kprintfd("BDVirtualDevice::blocks2read %d\n", blocks2read );
   char *my_buffer = (char *) kmalloc( blocks2read*block_size_*sizeof(char) );
   BDRequest * bd = 
   new BDRequest(0, BDRequest::BD_READ, blockoffset, blocks2read, my_buffer);
   addRequest ( bd );
   
   //kprintfd("BDVirtualDevice::request added\n" );
   
   while( bd->getStatus() == BDRequest::BD_QUEUED && 
          jiffies++ < 50000 );
          
   if( bd->getStatus() != BDRequest::BD_DONE )
   {
     //kprintfd("BDVirtualDevice::!done\n" );
     kfree( my_buffer );
     delete bd;
     return -1;
   }

   //kprintfd("BDVirtualDevice::done\n" );
   memcpy( buffer, my_buffer + (offset%block_size_), size );
   //kprintfd("BDVirtualDevice::memcpied\n" );
   kfree( my_buffer );
   delete bd;
   //kprintfd("BDVirtualDevice::returning\n" );
   return size;
};

int32 BDVirtualDevice::writeData(int32 offset, int32 size, const char *buffer)
{
   //kprintfd("BDVirtualDevice::writeData\n");
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
