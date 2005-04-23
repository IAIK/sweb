//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.cpp,v 1.7 2005/04/23 18:13:27 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: FrameBufferConsole.cpp,v $
//  Revision 1.5  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.4  2005/04/22 20:18:52  nomenquis
//  compile fixes
//
//  Revision 1.2  2005/04/22 18:23:16  nomenquis
//  massive cleanups
//
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#include "FrameBufferConsole.h"
#include "ArchCommon.h"
#include "Terminal.h"
#include "image.h"

FrameBufferConsole::FrameBufferConsole()
{
  x_res_ = ArchCommon::getVESAConsoleWidth();
  y_res_ = ArchCommon::getVESAConsoleHeight();
  bits_per_pixel_ = ArchCommon::getVESAConsoleBitsPerPixel();
  bytes_per_pixel_ = bits_per_pixel_ / 8;
  terminal_ = new Terminal(this,consoleGetNumColumns(),consoleGetNumRows());
}

void FrameBufferConsole::consoleClearScreen()
{
  uint8 *lfb = (uint8*)ArchCommon::getVESAConsoleLFBPtr();
  uint32 i,k;
  for (i=0;i<x_res_*y_res_*bytes_per_pixel_;++i)
  {
    lfb[i]=0;
  }
  uint32 off = (x_res_ - logo.width) / 2;
  
  for (i=0;i<logo.height;++i)
  {
    for (k=0;k<logo.width;++k)
    {
      setPixel(off+k,(y_res_-65+i),logo.pixel_data[(i*logo.width+k)*3],logo.pixel_data[(i*logo.width+k)*3+1],logo.pixel_data[(i*logo.width+k)*3+2]);
    }
  }
}


uint32 FrameBufferConsole::consoleGetNumRows() const
{
  return (y_res_-65) / 16;
}

uint32 FrameBufferConsole::consoleGetNumColumns() const
{
  return x_res_ / 8; 
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

uint32 FrameBufferConsole::consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)
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



uint32 FrameBufferConsole::setAsCurrent()
{
  return 0;
}

uint32 FrameBufferConsole::unsetAsCurrent()
{
  return 0;
}

void FrameBufferConsole::consoleScrollUp()
{
  pointer fb = ArchCommon::getVESAConsoleLFBPtr();
  ArchCommon::memcpy(fb, fb+(consoleGetNumColumns()*4*8*16),
    (consoleGetNumRows()-1)*consoleGetNumColumns()*4*8*16);
  ArchCommon::bzero(fb+((consoleGetNumRows()-1)*consoleGetNumColumns()*4*8*16),consoleGetNumColumns()*4*8*16);
  
}
