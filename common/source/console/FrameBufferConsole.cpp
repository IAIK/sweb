//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.cpp,v 1.1 2005/04/22 17:21:41 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------


#include "FrameBufferConsole.h"

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
static void kkkk(char *mesg)
{
  uint8 * fb = (uint8*)0xC00B8000;
  unsigned i=0;
  while (mesg && *mesg)
  {
    fb[i++] = *mesg++;
    fb[i++] = 0x9f;
  }
  for (;;);
}

uint32 FrameBufferConsole::setAsCurrent()
{
  kkkk("I'm the current console");
  return 0;
}

uint32 FrameBufferConsole::unsetAsCurrent()
{
  return 0;
}
