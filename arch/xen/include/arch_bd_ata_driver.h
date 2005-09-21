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
*    $Id: arch_bd_ata_driver.h,v 1.1 2005/09/21 02:18:58 rotho Exp $
*    $Log: arch_bd_ata_driver.h,v $
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
#include "FiFo.h"

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
    ~ATADriver();
    
    int32 readSector     ( uint32, uint32, void * );
    int32 writeSector    ( uint32, uint32, void * );
        
    uint32 getNumSectors ( void )     { return numsec; };
    uint32 getSectorSize ( void )     { return 512; };
    
    void serviceIRQ      ( void );
    void testIRQ         ( );

    uint32 HPC, SPT;        // HEADS PER CYLINDER and SECTORS PER TRACK
    
  private:  
    
    uint16           dd [256];    // read buffer if we need one
    uint32           dd_off;      // read buffer counter

    uint32           numsec;

    uint16           port;
    uint16           drive;
    
    uint32           jiffies;
    
    BD_ATA_MODES     mode; // mode see enum BD_ATA_MODES
    
    FiFo< BDRequest * > * request_queue_;
};

#endif

