//----------------------------------------------------------------------
//   $Id: Console.h,v 1.4 2005/04/23 18:13:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Console.h,v $
//  Revision 1.3  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.2  2005/04/22 19:18:14  nomenquis
//  w00t
//
//  Revision 1.1  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------


#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "types.h"

class Terminal;
class Console
{
friend class Terminal;
public:
 
  enum FOREGROUNDCOLORS
  {
    FG_BLACK=0,
    FG_BLUE,
    FG_GREEN,
    FG_CYAN,
    FG_RED,
    FG_MAGENTA,
    FG_BROWN,
    FG_WHITE,
    FG_DARK_GREY,
    FG_BRIGHT_BLUE,
    FG_BRIGHT_GREEN,
    FG_BRIGHT_CYAN,
    FG_PINK,
    FG_BRIGHT_MAGENTA,
    FG_YELLOW,
    FG_BRIGHT_WHITE 
  };

  /** this is insane, usually we should only have 8 background colors, but hey, what do i know? */
  enum BACKGROUNDCOLORS
  {
    BG_BLACK=0,
    BG_BLUE,
    BG_GREEN,
    BG_CYAN,
    BG_RED,
    BG_MAGENTA,
    BG_BROWN,
    BG_WHITE,
    BG_DARK_GREY,
    BG_BRIGHT_BLUE,
    BG_BRIGHT_GREEN,
    BG_BRIGHT_CYAN,
    BG_PINK,
    BG_BRIGHT_MAGENTA,
    BG_YELLOW,
    BG_BRIGHT_WHITE
  };

  void write(uint8 character);
  void writeString(uint8 const *string);
  void writeBuffer(uint8 const *buffer, size_t len);

  uint32 getNumRows() const;
  uint32 getNumColumns() const;

  void clearScreen();
  
  uint32 setCharacter(uint32 const &row, uint32 const&column, uint8 const &character);
  
  void setForegroundColor(FOREGROUNDCOLORS const &color);
  void setBackgroundColor(BACKGROUNDCOLORS const &color);
 
  
protected:
  
  virtual void consoleClearScreen()=0;
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)=0;
  virtual uint32 consoleGetNumRows() const = 0;
  virtual uint32 consoleGetNumColumns() const = 0;
  virtual void consoleScrollUp()=0;

  Terminal *terminal_;
};



#endif
