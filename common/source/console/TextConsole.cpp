//----------------------------------------------------------------------
//   $Id: TextConsole.cpp,v 1.3 2005/04/23 15:58:32 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: TextConsole.cpp,v $
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

static void kkkk(char *mesg)
{
  uint8 * fb = (uint8*)0xC00B8000;
  uint32 i=0;
  while (mesg && *mesg)
  {
    fb[i++] = *mesg++;
    fb[i++] = 0x9f;
  }
  for (;;);
}

uint32 TextConsole::setAsCurrent()
{
  kkkk("I'm the current console");
  return 0;
}

uint32 TextConsole::unsetAsCurrent()
{
  return 0;
}
