/********************************************************************
*
*    $Id: arch_bd_virtual_device.h,v 1.4 2005/11/20 21:18:08 nelles Exp $
*    $Log: arch_bd_virtual_device.h,v $
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

#ifndef _BD_VIRTUAL_DEVICE_
#define _BD_VIRTUAL_DEVICE_

#include "types.h"
#include "string.h"

#include "arch_bd_request.h"
#include "arch_bd_driver.h"

#include "fs/PointList.h"
#include "fs/Inode.h"
#include "fs/ramfs/RamFsFile.h"
#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"

class BDVirtualDevice : public Inode
{
  public:

    BDVirtualDevice( BDDriver * driver, uint32 offset, uint32 num_blocks, uint32 block_size, char *name, bool writable);
  
    void BDVirtualDevice::addRequest(BDRequest * command);
    
    uint32    getBlockSize()                  { return block_size_; };
    uint32    getDeviceNumber()               { return dev_number_; };
    BDDriver* getDriver()                     { return driver_; };
    char*     getName()                       { return name_; };
    uint32    getNumBlocks()                  { return num_blocks_; };

    
//// -----------------------------------------------------------------
//// Inode functions

    virtual int32 readData(int32 offset, int32 size, char *buffer);
    virtual int32 writeData(int32 offset, int32 size, const char *buffer);
        
    int32 mknod(Dentry *dentry)
    {
      if(dentry == 0)
        return -1;
    
      i_dentry_ = dentry;
      dentry->setInode(this);
      return 0;
    } 
    
    int32 create(Dentry *dentry)
    {
      return(mknod(dentry));
    }
    
    int32 mkfile(Dentry *dentry)
    {
      return(mknod(dentry));
    }
    
    File* link(uint32 flag)
    {
      File* file = (File*)(new RamFsFile(this, i_dentry_, flag));
      i_files_.pushBack(file);
      return file;
    }
  
    int32 unlink(File* file)
    {
      int32 tmp = i_files_.remove(file);
      delete file;
      return tmp;
    }    
    
//// End Inode functions
//// -----------------------------------------------------------------

    
    void    setDeviceNumber( uint32 number ) { dev_number_ = number; };
    
     
  private:
    BDVirtualDevice();
    
    uint32 dev_number_;
    uint32 block_size_;
    uint32 num_blocks_;
    uint32 offset_;
    bool writable_;
    char *name_;
    BDDriver * driver_;
};

#endif
