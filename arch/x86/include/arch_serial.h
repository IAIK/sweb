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
//   $Id: arch_serial.h,v 1.3 2005/09/20 21:14:31 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_serial.h,v $
//
//--------------------------------------------------------------------

#ifndef _ARCH_SERIAL_H_
#define _ARCH_SERIAL_H_

#include "types.h"

/// \class ArchSerialInfo
/// \brief Class that contains architecture specific parameters for serial port communication

class ArchSerialInfo
{
public:
  uint16 base_port;  ///< The base IO port 
  uint8  uart_type;  ///< Type of the connected UART
  uint8  irq_num;    ///< IRQ number
};

/// \class SC
/// \brief Class that contains constants. Cleaner OO solution than #defining constants.

class SC
{
public:
  static uint32 IER;  ///< UART Interrupt Enable Register
  static uint32 IIR;  ///< UART Interrupt Indetification Register
  static uint32 FCR;  ///< UART FIFO Control Register
  static uint32 LCR;  ///< UART Line Control Register
  static uint32 MCR;  ///< UART Modem Control Register
  static uint32 LSR;  ///< UART Line Status Register
  static uint32 MSR;  ///< UART Modem Status Register
  
  static uint32 UART_16650A;  ///< Most common UART with FIFO buffers
  static uint32 UART_16650;  ///< Old type UART with FIFO buffers that do not work
  static uint32 UART_OLD;    ///< Old type UART without FIFO buffers
  
  static uint32 MAX_ARCH_PORTS;  ///< Maximum serial ports registered in BIOS
};



#endif
