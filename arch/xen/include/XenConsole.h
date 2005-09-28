//----------------------------------------------------------------------
//   $Id: XenConsole.h,v 1.1 2005/09/28 16:35:43 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: XenConsole.h,v $
//----------------------------------------------------------------------

#ifndef _XENCONSOLE_H_
#define _XENCONSOLE_H_

#include "console/Console.h"

class XenConsole : public Console
{
public:
  
  XenConsole(uint32 num_terminals);

  virtual void Run();
  
  void handleKey( uint32 );
  
  uint32 remap( uint32 ); // this should be moved to terminal
  
  bool isDisplayable( uint32 );
  
  bool isLetter( uint32 );
  bool isNumber( uint32 );
  

private:
  
  virtual void consoleClearScreen();
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state);
  virtual uint32 consoleGetNumRows() const;
  virtual uint32 consoleGetNumColumns() const;
  virtual void consoleScrollUp();
  virtual void consoleSetForegroundColor(FOREGROUNDCOLORS const &color);
  virtual void consoleSetBackgroundColor(BACKGROUNDCOLORS const &color);

};

#endif
