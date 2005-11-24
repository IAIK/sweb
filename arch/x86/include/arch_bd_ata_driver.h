// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Nebojsa Simic 
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/********************************************************************
*
*    $Id: arch_bd_ata_driver.h,v 1.7 2005/11/24 23:37:02 nelles Exp $
*    $Log: arch_bd_ata_driver.h,v $
*    Revision 1.6  2005/11/20 21:18:08  nelles
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
*    Revision 1.5  2005/10/24 21:28:04  nelles
*
*     Fixed block devices. I think.
*
*     Committing in .
*
*     Modified Files:
*
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/source/InterruptUtils.cpp
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/xen/source/arch_bd_ide_driver.cpp
*
*     	common/source/kernel/SpinLock.cpp
*     	common/source/kernel/Thread.cpp utils/bochs/bochsrc
*
*    Revision 1.4  2005/10/02 12:27:55  nelles
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
*    Revision 1.3  2005/09/20 21:14:31  nelles
*
*
*    Some comments added
*
*     ----------------------------------------------------------------------
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

#ifndef _ATA_DEVICE_DRIVER_
#define _ATA_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "arch_bd_driver.h"
#include "arch_bd_io.h"
#include "Queue.h"

class ATADriver : public BDDriver, bdio
{
  public:
  
    typedef enum BD_ATA_MODE_ {
      BD_PIO_NO_IRQ,
      BD_PIO,
      BD_DMA,
      BD_UDMA
    } BD_ATA_MODES;
    
  
    uint32 addRequest( BDRequest * );
    
    ATADriver( uint16 baseport, uint16 getdrive, uint16 irqnum );
    virtual ~ATADriver() {};
    
	int32 rawReadSector  ( uint32, uint32, void * );
    int32 readSector     ( uint32, uint32, void * );
    int32 writeSector    ( uint32, uint32, void * );
        
    uint32 getNumSectors ( void )     { return numsec; };
    uint32 getSectorSize ( void )     { return 512; };
    
    void serviceIRQ      ( void );
    void testIRQ         ( );
	
	bool waitForController( bool resetIfFailed );

    uint32 HPC, SPT;        // HEADS PER CYLINDER and SECTORS PER TRACK
    
  private:  
    
    uint16           dd [256];    // read buffer if we need one
    uint32           dd_off;      // read buffer counter

    uint32           numsec;

    uint16           port;
    uint16           drive;
    
    uint32           jiffies;
    
    BD_ATA_MODES     mode; // mode see enum BD_ATA_MODES
    
    BDRequest *request_list_;
    BDRequest *request_list_tail_;
};

#endif
