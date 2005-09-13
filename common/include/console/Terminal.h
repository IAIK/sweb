//----------------------------------------------------------------------
//  $Id: Terminal.h,v 1.4 2005/09/13 15:00:51 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Terminal.h,v $
//  Revision 1.3  2005/07/27 10:04:26  btittelbach
//  kprintf_nosleep and kprintfd_nosleep now works
//  Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//  Revision 1.2  2005/07/24 17:02:59  nomenquis
//  lots of changes for new console stuff
//
//  Revision 1.1  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//----------------------------------------------------------------------



#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "types.h"
#include "Console.h"

class Console;
  
class Terminal
{
friend class Console;
public:

  Terminal(Console *console, uint32 num_columns, uint32 num_rows);
  
  void write(char character);
  void writeString(char const *string);
  void writeBuffer(char const *buffer, size_t len);

  void setForegroundColor(Console::FOREGROUNDCOLORS const &color);
  void setBackgroundColor(Console::BACKGROUNDCOLORS const &color);

  void writeInternal(char character);

  bool isLockFree()
  {
    return mutex_.isFree();
  }

protected:
  
  void setAsActiveTerminal();
  void unSetAsActiveTerminal();

private:

  void clearScreen();
  void fullRedraw();
  uint32 getNumRows() const;
  uint32 getNumColumns() const;

  uint32 setCharacter(uint32 row,uint32 column, uint8 character);


  void scrollUp();

  Console *console_;
  uint32 num_columns_;
  uint32 num_rows_;
  uint32 len_;
  uint8 *characters_;
  uint8 *character_states_;

  uint32 current_column_;
  uint8 current_state_;

  uint8 active_;
  
  Mutex mutex_;
};


#endif
