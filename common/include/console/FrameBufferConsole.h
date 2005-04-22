//----------------------------------------------------------------------
//   $Id: FrameBufferConsole.h,v 1.1 2005/04/22 17:21:40 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------


#ifndef _FRAMEBUFFERCONSOLE_H_
#define _FRAMEBUFFERCONSOLE_H_

#include "Console.h"


class FrameBufferConsole : public Console
{
public:
  
  FrameBufferConsole();

  virtual uint32 getNumRows() const=0;
  virtual uint32 getNumColumns() const=0;

  virtual void clear()=0;
  
  virtual uint32 setCharacter(uint32 const&column, uint32 const &row, uint8 const &character)=0;

  // make this an enum
  virtual void setForegroundColor(uint32 const &color)=0;
  virtual void setBackgroundColor(uint32 const &color)=0;
 
  virtual uint32 setAsCurrent();
  virtual uint32 unsetAsCurrent();

};





#endif
