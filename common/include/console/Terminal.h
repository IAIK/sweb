//----------------------------------------------------------------------
//  $Id: Terminal.h,v 1.1 2005/04/23 15:58:32 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------



#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "types.h"
#include "Console.h"

class Console;
  
class Terminal
{
public:

  Terminal(Console *console, uint32 num_columns, uint32 num_rows);
  
  void write(uint8 character);
  void writeString(uint8 const *string);
  void writeBuffer(uint8 const *buffer, size_t len);

  void clearScreen();

  uint32 getNumRows() const;
  uint32 getNumColumns() const;

  uint32 setCharacter(uint32 row,uint32 column, uint8 character);

  void setForegroundColor(Console::FOREGROUNDCOLORS const &color);
  void setBackgroundColor(Console::BACKGROUNDCOLORS const &color);

private:
  
  void scrollUp();

  Console *console_;
  uint32 num_columns_;
  uint32 num_rows_;
  uint32 len_;
  uint8 *characters_;
  uint8 *character_states_;

  uint32 current_column_;
  uint8 current_state_;

};


#endif
