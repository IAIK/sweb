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

#include "arch_serial.h"
#include "serial.h"
#include "string.h"

#include "debug_bochs.h"

SerialManager * SerialManager::instance_ = 0;

SerialManager::SerialManager ( ) : num_ports( 0 )
{
  
};

SerialManager::~SerialManager ( )
{
  
};

uint32 SerialManager::get_num_ports()
{
  return num_ports;
};

uint32 SerialManager::do_detection( uint32 is_paging_set_up )
{
  uint16 * bios_sp_table;
  
  if (is_paging_set_up)
    bios_sp_table = (uint16 *) 0xC0000400;
  else
    bios_sp_table = (uint16 *) 0x00000400;
    
  uint32 i = 0;
  
  for( ; i < SC::MAX_ARCH_PORTS; i++, bios_sp_table++ )
  {
    
    if( *bios_sp_table != 0x00 )
    {

      ArchSerialInfo * archInfo = new ArchSerialInfo();
      archInfo->base_port = *bios_sp_table;
      archInfo->uart_type = SC::UART_OLD;  // TODO: UART type detection
      archInfo->irq_num = 4 - i%2;
      serial_ports[ num_ports ] = new SerialPort( *archInfo );
      
      uint8 sp_name[] = { 'S', 'P', num_ports + '1', '\0' };
            
      strcpy( (char *) serial_ports[ num_ports ]->friendly_name, (char *) sp_name );
      
      num_ports++;
      
    }
  }
  
  return num_ports;     
}

void SerialManager::service_irq( uint32 irq_num )
{
  uint32 i = num_ports;
  
  for( i = 0;i < num_ports; i++)
    if( serial_ports[ i ]->get_info().irq_num == irq_num )
      serial_ports[ i ]->irq_handler();
}
