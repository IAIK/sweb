/**
 * @file SerialConsole.cpp not implemented
 */
/*
#include "SerialConsole.h"
#include "Terminal.h"
#include "ArchCommon.h"

#include "arch_serial.h"
#include "drivers/serial.h"

#include "debug_bochs.h"

#define BOCHS_DEBUG  1

SerialConsole::SerialConsole( uint32 port_num ) : con_cpos_(0)
{
  port_num_ = port_num;

  terminal_ = new Terminal(this,consoleGetNumColumns(),consoleGetNumRows());
}


uint32 SerialConsole::consoleGetNumRows() const
{
  return 1;
}

uint32 SerialConsole::consoleGetNumColumns() const
{
  return 80;
}

void SerialConsole::consoleClearScreen()
{
  consoleScrollUp();
  return;
}

uint32 SerialConsole::consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)
{
  uint32 bytes_written = 0;
  uint8 ch = character;
  uint8 sp = ' ';


  for(; column > con_cpos_; con_cpos_++ )
#if defined(SERIAL_DEBUG)
        SerialManager::getInstance()->serial_ports[port_num_]->write( &sp, 1, bytes_written);
#elif defined(BOCHS_DEBUG)
      writeChar2Bochs( sp );
#else
        ;
#endif



#if defined(SERIAL_DEBUG)
  SerialManager::getInstance()->serial_ports[port_num_]->write( &ch, 1, bytes_written);
#elif defined(BOCHS_DEBUG)
  writeChar2Bochs( ch );
#else
  ;
#endif

  con_cpos_++;

  return !(bytes_written);
}

void SerialConsole::consoleScrollUp()
{
  uint32 i = 80 - con_cpos_;
  uint8 sp = ' ';
  uint32 bytes_written = 0;

#if defined(SERIAL_DEBUG)
  while(i--)
    SerialManager::getInstance()->serial_ports[port_num_]->write( &sp, 1, bytes_written);
#elif defined(BOCHS_DEBUG)
  writeChar2Bochs( '\r' );
  writeChar2Bochs( '\n' );
#else
        ;
#endif

  con_cpos_ = 0;

  return;
}


uint32 SerialConsole::setAsCurrent()
{
  return 0;
}

uint32 SerialConsole::unsetAsCurrent()
{
  return 0;
}

void SerialConsole::consoleSetForegroundColor(FOREGROUNDCOLORS const &color)
{

}

void SerialConsole::consoleSetBackgroundColor(BACKGROUNDCOLORS const &color)
{

}
*/
