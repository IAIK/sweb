//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.cpp,v 1.4 2005/04/22 20:18:52 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: FrameBufferConsole.cpp,v $
//  Revision 1.2  2005/04/22 18:23:16  nomenquis
//  massive cleanups
//
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#include "FrameBufferConsole.h"
#include "ArchCommon.h"

FrameBufferConsole::FrameBufferConsole()
{
  x_res_ = ArchCommon::getVESAConsoleWidth();
  y_res_ = ArchCommon::getVESAConsoleHeight();
  bits_per_pixel_ = ArchCommon::getVESAConsoleBitsPerPixel();
  bytes_per_pixel_ = bits_per_pixel_ / 8;
}

uint32 FrameBufferConsole::getNumRows()const
{
  return y_res_ / 16;
}

uint32 FrameBufferConsole::getNumColumns()const
{
  return x_res_ / 8;
}

void FrameBufferConsole::clear()
{
  
}
extern uint8 fontdata_sun8x16[];

void FrameBufferConsole::setPixel(uint32 x,uint32 y,uint8 r,uint8 g,uint8 b)
{
  uint8 *lfb = (uint8*)ArchCommon::getVESAConsoleLFBPtr();
  uint32 offset = (x + y*x_res_) * 4;
  lfb[offset + 0] = b;
  lfb[offset + 1] = g;
  lfb[offset + 2] = r;
  lfb[offset + 3] = 0;
  
}

uint32 FrameBufferConsole::setCharacter(uint32 const&column, uint32 const &row, uint8 const &character)
{
  uint32 i,k;
  
  
  uint32 character_index = character * 16;
  
  for (i=0;i<16;++i)
  {
    
    for (k=0;k<8;++k)
    {
      // find out the bit we want to draw
      uint8 temp = fontdata_sun8x16[character_index+i];
      temp &= 1<<(7-k);
      if (temp)
      {
        setPixel(column * 8 + k, row * 16 + i,128,128,128);
      }
    }
    
  }
  
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
  uint32 i;
  
  setCharacter(0+40,15,'S');
  setCharacter(1+40,15,'W');
  setCharacter(2+40,15,'E');
  setCharacter(3+40,15,'B');
  setCharacter(4+40,15,' ');
  setCharacter(5+40,15,'i');
  setCharacter(6+40,15,'s');
  setCharacter(7+40,15,' ');
  setCharacter(8+40,15,'3');
  setCharacter(9+40,15,'1');
  setCharacter(10+40,15,'3');
  setCharacter(11+40,15,'3');
  setCharacter(12+40,15,'7');
  
  setCharacter(0+46,20,'w');
  setCharacter(1+46,20,'0');
  setCharacter(2+46,20,'0');
  setCharacter(3+46,20,'t');
  setCharacter(4+46,20,'!');

  for (i=0;i<256;++i)
  {
    setCharacter(i%getNumColumns(),i/getNumColumns()+30,i);
  }
  for (;;);
  return 0;
}

uint32 FrameBufferConsole::unsetAsCurrent()
{
  return 0;
}
