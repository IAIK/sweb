//----------------------------------------------------------------------
//   $Id: TextConsole.cpp,v 1.4 2005/04/23 18:13:27 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: TextConsole.cpp,v $
//  Revision 1.3  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.2  2005/04/22 19:43:04  nomenquis
//   more poison added
//
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#include "TextConsole.h"
#include "Terminal.h"
#include "ArchCommon.h"

TextConsole::TextConsole()
{
  terminal_ = new Terminal(this,consoleGetNumColumns(),consoleGetNumRows());
}

uint32 TextConsole::consoleGetNumRows()const
{
  return 25;
}

uint32 TextConsole::consoleGetNumColumns()const
{
  return 80;
}

void TextConsole::consoleClearScreen()
{
  char *fb = (char*)ArchCommon::getFBPtr();
  uint32 i;
  for (i=0;i<consoleGetNumRows()*consoleGetNumColumns()*2;++i)
  {
    fb[i]=0;
  }
}

uint32 TextConsole::consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)
{
  char *fb = (char*)ArchCommon::getFBPtr();
  fb[(column + row*consoleGetNumColumns())*2] = character;
  fb[(column + row*consoleGetNumColumns())*2+1] = state;
  
  return 0;
}

void TextConsole::consoleScrollUp()
{
  pointer fb = ArchCommon::getFBPtr();
  ArchCommon::memcpy(fb, fb+(consoleGetNumColumns()*2),
    (consoleGetNumRows()-1)*consoleGetNumColumns()*2);
  ArchCommon::bzero(fb+((consoleGetNumRows()-1)*consoleGetNumColumns()*2),consoleGetNumColumns()*2);
}


uint32 TextConsole::setAsCurrent()
{
  return 0;
}

uint32 TextConsole::unsetAsCurrent()
{
  return 0;
}
