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
//   $Id: arch_bd_driver.h,v 1.3 2005/10/02 12:27:55 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_bd_driver.h,v $
//  Revision 1.2  2005/09/20 21:14:31  nelles
//
//
//  Some comments added
//
//   ----------------------------------------------------------------------
//
//
//--------------------------------------------------------------------

#ifndef _BD_DEVICE_DRIVER_
#define _BD_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "FiFo.h"

class BDDriver
{
  public:
    virtual ~BDDriver() {};
    virtual uint32 addRequest( BDRequest * ) = 0;
/*    {
      return 0xFFFFFFFF;
    };*/
    
    virtual int32 readSector ( uint32, uint32, void * ) = 0;
/*    {
      return -1;
    };*/
    
    virtual int32 writeSector ( uint32, uint32, void *  ) = 0;
//     {
//       return -1;
//     };

    virtual uint32 getNumSectors( ) = 0;
//     {
//       return 0;
//     };

    virtual uint32 getSectorSize( ) = 0;
/*    {
      return 0;
    };*/
    
    virtual void serviceIRQ( void ) = 0;

    uint16 irq;
};

#endif

