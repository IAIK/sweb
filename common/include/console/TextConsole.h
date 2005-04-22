//----------------------------------------------------------------------
//   $Id: TextConsole.h,v 1.1 2005/04/22 17:21:40 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef _TEXTCONSOLE_H_
#define _TEXTCONSOLE_H_

#include "Console.h"

class TextConsole : public Console
{
public:
  
  TextConsole();

  virtual uint32 getNumRows() const;
  virtual uint32 getNumColumns() const;

  virtual void clear();
  
  virtual uint32 setCharacter(uint32 const&column, uint32 const &row, uint8 const &character);

  // make this an enum
  virtual void setForegroundColor(uint32 const &color);
  virtual void setBackgroundColor(uint32 const &color);
  
  virtual uint32 setAsCurrent();
  virtual uint32 unsetAsCurrent();

};





#endif
