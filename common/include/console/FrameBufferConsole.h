//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.h,v 1.4 2005/04/23 15:58:32 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: FrameBufferConsole.h,v $
//  Revision 1.3  2005/04/22 19:18:14  nomenquis
//  w00t
//
//  Revision 1.2  2005/04/22 18:23:04  nomenquis
//  massive cleanups
//
//  Revision 1.1  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#ifndef _FRAMEBUFFERCONSOLE_H_
#define _FRAMEBUFFERCONSOLE_H_

#include "Console.h"


class FrameBufferConsole : public Console
{
public:
  
  FrameBufferConsole();
 
  virtual uint32 setAsCurrent();
  virtual uint32 unsetAsCurrent();

protected:
  
  virtual void consoleClearScreen();
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state);
  virtual uint32 consoleGetNumRows() const;
  virtual uint32 consoleGetNumColumns() const;

private:
  
  void setPixel(uint32 x,uint32 y,uint8 r,uint8 g,uint8 b);
  uint32 x_res_;
  uint32 y_res_;
  uint32 bits_per_pixel_;
  uint32 bytes_per_pixel_;
};





#endif
