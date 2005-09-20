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

//----------------------------------------------------------------------
//   $Id: arch_bd_driver.h,v 1.2 2005/09/20 21:14:31 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_bd_driver.h,v $
//
//--------------------------------------------------------------------

#ifndef _BD_DEVICE_DRIVER_
#define _BD_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "FiFo.h"

class BDDriver
{
  public:
    uint32 addRequest( BDRequest * ) 
    {
      return 0xFFFFFFFF;
    };
    
    int32 readSector ( uint32, uint32 )
    {
      return -1;
    };
    
    int32 writeSector ( uint32, uint32, void *  )
    {
      return -1;
    };

    uint32 getNumSectors( )
    {
      return 0;
    };

    uint32 getSectorSize( )
    {
      return 0;
    };
    
    void serviceIRQ( void )
    {
      return;
    };

    uint16 irq;
};

#endif

