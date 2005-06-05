//----------------------------------------------------------------------
//   $Id: SerialConsole.h
//----------------------------------------------------------------------


#ifndef _SERIAL_CONSOLE_H_
#define _SERIAL_CONSOLE_H_

#include "Console.h"

class SerialConsole : public Console
{
public:
  
  SerialConsole( uint32 port_num );

  
  virtual uint32 setAsCurrent();
  virtual uint32 unsetAsCurrent();

private:

  uint32 con_cpos_;
  uint32 port_num_;

  virtual void consoleClearScreen();
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state);
  virtual uint32 consoleGetNumRows() const;
  virtual uint32 consoleGetNumColumns() const;
  virtual void consoleScrollUp();
  virtual void consoleSetForegroundColor(FOREGROUNDCOLORS const &color);
  virtual void consoleSetBackgroundColor(BACKGROUNDCOLORS const &color);

};





#endif
