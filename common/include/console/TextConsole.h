//----------------------------------------------------------------------
//   $Id: TextConsole.h,v 1.5 2005/07/24 17:02:59 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: TextConsole.h,v $
//  Revision 1.4  2005/04/23 20:08:26  nomenquis
//  updates
//
//  Revision 1.3  2005/04/23 18:13:27  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
//  Revision 1.2  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.1  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#ifndef _TEXTCONSOLE_H_
#define _TEXTCONSOLE_H_

#include "Console.h"

class TextConsole : public Console
{
public:
  
  TextConsole(uint32 num_terminals);


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
