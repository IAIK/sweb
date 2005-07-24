//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.cpp,v 1.10 2005/07/24 17:02:59 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: FrameBufferConsole.cpp,v $
//  Revision 1.9  2005/04/26 17:03:27  nomenquis
//  16 bit framebuffer hack
//
//  Revision 1.8  2005/04/23 20:08:26  nomenquis
//  updates
//
//  Revision 1.7  2005/04/23 18:13:27  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
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
/*
FrameBufferConsole::FrameBufferConsole()
{
  x_res_ = ArchCommon::getVESAConsoleWidth();
  y_res_ = ArchCommon::getVESAConsoleHeight();
  bits_per_pixel_ = ArchCommon::getVESAConsoleBitsPerPixel();
  bytes_per_pixel_ = bits_per_pixel_ / 8;
  terminal_ = new Terminal(this,consoleGetNumColumns(),consoleGetNumRows());
  consoleSetForegroundColor(Console::FG_GREEN);
  consoleSetBackgroundColor(Console::BG_BLACK);
}

void FrameBufferConsole::consoleClearScreen()
{
  uint8 *lfb = (uint8*)ArchCommon::getVESAConsoleLFBPtr();
  uint32 i,k;
  
  ArchCommon::bzero((pointer)lfb,x_res_*y_res_*bytes_per_pixel_);

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
  uint16 *lfb = (uint16*)ArchCommon::getVESAConsoleLFBPtr();
  uint32 offset = (x + y*x_res_);
  uint16 color=(b>>3);
  color|=(g>>2)<<5;
  color|=(r>>3)<<11;
  
  lfb[offset] = color;
  /*
  lfb[offset + 0] = b;
  lfb[offset + 1] = g;
  lfb[offset + 2] = r;
  lfb[offset + 3] = 0;
  */
  /*
}

uint32 FrameBufferConsole::consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)
{
  uint32 i,k;
  uint32 character_index = character * 16;
  
  uint16 *lfb = (uint16*)ArchCommon::getVESAConsoleLFBPtr();

  uint32 top_left_pixel = column*8 + row*16*x_res_;
  
  for (i=0;i<16;++i)
  {
    
    for (k=0;k<8;++k)
    {
      // find out the bit we want to draw
      uint8 temp = fontdata_sun8x16[character_index+i];
      temp &= 1<<(7-k);
      if (temp)
      {
        lfb[top_left_pixel + k + i*x_res_] = current_foreground_color_;
      }
      else
      {
        lfb[top_left_pixel + k + i*x_res_] = current_background_color_;
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
  ArchCommon::memcpy(fb, fb+(consoleGetNumColumns()*bytes_per_pixel_*8*16),
    (consoleGetNumRows()-1)*consoleGetNumColumns()*bytes_per_pixel_*8*16);
  ArchCommon::bzero(fb+((consoleGetNumRows()-1)*consoleGetNumColumns()*bytes_per_pixel_*8*16),consoleGetNumColumns()*bytes_per_pixel_*8*16);
  
}

void FrameBufferConsole::consoleSetForegroundColor(FOREGROUNDCOLORS const &color)
{
  uint8 r,g,b;
  r = 0;
  g = 255;
  b = 0;
  
  current_foreground_color_ = (r<<16) + (g<<8) + (b); 
  
}
void FrameBufferConsole::consoleSetBackgroundColor(BACKGROUNDCOLORS const &color)
{
  uint8 r,g,b;
  r = 0;
  g = 0;
  b = 0;
  current_background_color_ = (r<<16) + (g<<8) + (b); 
}
*/
