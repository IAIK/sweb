//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.cpp,v 1.2 2005/04/22 18:23:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: FrameBufferConsole.cpp,v $
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#include "FrameBufferConsole.h"
#include "ArchCommon.h"

FrameBufferConsole::FrameBufferConsole()
{
}

uint32 FrameBufferConsole::getNumRows()const
{
  return 20;
}

uint32 FrameBufferConsole::getNumColumns()const
{
  return 80;
}

void FrameBufferConsole::clear()
{
  
}

uint32 FrameBufferConsole::setCharacter(uint32 const&column, uint32 const &row, uint8 const &character)
{
  return 0;
}


void FrameBufferConsole::setForegroundColor(uint32 const &color)
{
  
}

void FrameBufferConsole::setBackgroundColor(uint32 const &color)
{
  
}

uint32 FrameBufferConsole::setAsCurrent()
{
  uint8 *lfb = (uint8*)ArchCommon::getVESAConsoleLFBPtr();
  uint32 i;
  for (i=0;i<100000000;++i)
  {
    lfb[i] = i%256;
  }
  for (;;);
  return 0;
}

uint32 FrameBufferConsole::unsetAsCurrent()
{
  return 0;
}
