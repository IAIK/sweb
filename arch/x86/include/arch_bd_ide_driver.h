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
*    $Id: arch_bd_ide_driver.h,v 1.4 2005/10/02 12:27:55 nelles Exp $
*    $Log: arch_bd_ide_driver.h,v $
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

#ifndef _IDE_BUS_DEVICE_DRIVER_
#define _IDE_BUS_DEVICE_DRIVER_

#include "arch_bd_ata_driver.h"
#include "arch_bd_io.h"

class IDEDriver : public bdio
{
public:
  IDEDriver()
  {
    doDeviceDetection();
  };
  
  ~IDEDriver()
  {
  };
   
   typedef struct fdisk_partition
  {
    uint8 bootid;   // bootable?  0=no, 128=yes
    uint8 beghead;
    uint8 begcyl;
    uint8 begsect;
    uint8 systid;   // Operating System type indicator code 
    uint8 endhead;  
    uint8 endcyl;
    uint8 endsect;  
                              
    uint32 relsect;   // first sector relative to start of disk - We actually need only theese two params 
    uint32 numsect;   // number of sectors in partition 
  } FP;

  typedef struct master_boot_record 
  {
    uint8    bootinst[446];           // GRUB space
    uint8   parts[ 4*sizeof(FP) ];
    uint16  signature;                // set to 0xAA55 for PC MBR
  } MBR;
    
  int32    processMBR  ( ATADriver *, uint32, uint32, char* );
  uint32   doDeviceDetection ( );
};

#endif
