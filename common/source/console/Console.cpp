//----------------------------------------------------------------------
//  $Id: Console.cpp,v 1.1 2005/04/23 15:58:32 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "Console.h"
#include "Terminal.h"

    
uint32 Console::getNumRows() const
{
  return terminal_->getNumRows();
}

uint32 Console::getNumColumns() const
{
  return terminal_->getNumColumns();
}

void Console::clearScreen()
{
  terminal_->clearScreen();
}

uint32 Console::setCharacter(uint32 const &row, uint32 const&column, uint8 const &character)
{
  return terminal_->setCharacter(row,column,character);
}

void Console::write(uint8 character)
{
  terminal_->write(character);
}

void Console::writeString(uint8 const *string)
{
  terminal_->writeString(string);
}

void Console::writeBuffer(uint8 const *buffer, size_t len)
{
  terminal_->writeBuffer(buffer, len);
}

void Console::setForegroundColor(FOREGROUNDCOLORS const &color)
{
  terminal_->setForegroundColor(color);
}
void Console::setBackgroundColor(BACKGROUNDCOLORS const &color)
{
  terminal_->setBackgroundColor(color);
}
