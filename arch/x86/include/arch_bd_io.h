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
*    $Id: arch_bd_io.h,v 1.2 2005/09/20 21:14:31 nelles Exp $
*    $Log: arch_bd_io.h,v $
**********************************************************************/

#ifndef _BD_IO_
#define _BD_IO_

#include "types.h"
  
class bdio {
  public:
  // C Wrapers for reading and writing IO ports 
  // practicaly everything that exists in linux asm/io.h 
  // but without hardware abstractions, purely for x86 archi
  
  // The C wrappers are wrapped conviniently in a C++ class
  // each class that needs access to these functions can simply
  // inherit the bdio class or call them through bdio::method();
  
  // writes a byte to the IO port 
  static void outb( uint16 port, uint8 value) 
  {
      asm volatile ( "outb %b0, %1" : : "a" (value), "id" (port) );
  };
  
  // writes a byte to the IO port with slowdown */
  static void outbp( uint16 port, uint8 value) 
  {
      asm volatile ( "outb %b0, %1" : : "a" (value), "id" (port) );
      asm volatile ( "outb %al,$0x80" ); 
  };
  
  // reads a byte from the IO port */
  static uint8 inb ( uint16 port ) 
  { 
      uint8 _v; 
      asm volatile ( "inb %1, %0" : "=a" (_v) : "id" (port)  );
  
      return _v;
  };
  
  // reads a byte and slows down IO by trying to write to a nonexisting port
  // nice hack - Thanks Linus 
  
  static uint8 inbp ( uint16 port) 
  { 
      uint8 _v; 
      asm volatile ( "inb %1, %0" : "=a" (_v) : "id" (port)  );
      asm volatile ( "outb %al,$0x80" ); 
      return _v;
  };
  
  // writes a word to the IO port 
  static void outw( uint16 port, uint16 value) 
  {
      asm volatile ( "outw %w0, %1" : : "a" (value), "id" (port) );
  };
  
  // writes a word to the IO port with slowdown 
  static void outwp( uint16 port, uint16 value) 
  {
      asm volatile ( "outw %w0, %1" : : "a" (value), "id" (port) );
      asm volatile ( "outb %al,$0x80" ); 
  };
  
  // reads a word from the IO port 
  static uint16 inw ( uint16 port) 
  { 
      uint16 _res; 
      asm volatile ( "inw %1, %0" : "=a" (_res) : "id" (port)  );
  
      return _res;
  };
  
  // reads a word from the IO port with slowdown 
  static uint16 inwp ( uint16 port) 
  { 
      uint16 _res; 
      asm volatile ( "inw %1, %0" : "=a" (_res) : "id" (port)  );
      asm volatile ( "outb %al,$0x80" );
  
      return _res;
  };
  
  // (maybe) the functions should be implemented through macros 
  // like in linux, with faster input and output functions that detect 
  // if port is constant and lower than 0xFF 
  // check asm/io.h in linux  

};

#endif
