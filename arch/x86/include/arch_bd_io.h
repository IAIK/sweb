#ifndef _BD_IO_
#define _BD_IO_

#include "types.h"
  
class bdio {
  public:
  // C Wrapers for reading and writing IO ports 
  // practicaly everything that exists in linux asm/io.h 
  // but without hardware abstractons, purely for x86 archi 
  
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
