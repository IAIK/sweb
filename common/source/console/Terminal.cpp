//----------------------------------------------------------------------
//  $Id: Terminal.cpp,v 1.2 2005/04/23 18:13:27 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Terminal.cpp,v $
//  Revision 1.1  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//----------------------------------------------------------------------

#include "Terminal.h"
#include "Console.h"

Terminal::Terminal(Console *console, uint32 num_columns, uint32 num_rows):
  console_(console), num_columns_(num_columns), num_rows_(num_rows), len_(num_rows * num_columns),
  current_column_(0), current_state_(0x93)
{
  uint32 i;
  characters_ = new uint8[len_];
  character_states_ = new uint8[len_];
  clearScreen();
}
  
void Terminal::write(uint8 character)
{
  if (character == '\n')
  {
    scrollUp();
    current_column_ = 0;
  }
  else
  {
    setCharacter(num_rows_-1,current_column_,character);
    ++current_column_;
    if (current_column_ >= num_columns_)
    {
      // scroll up
      scrollUp();
      current_column_ = 0;
    }
  }
}

void Terminal::writeString(uint8 const *string)
{
  while (string && *string)
  {
    write(*string);
    ++string;
  }
}

void Terminal::writeBuffer(uint8 const *buffer, size_t len)
{
}

void Terminal::clearScreen()
{
  uint32 i;
  for (i=0;i<len_;++i)
  {
    characters_[i]=' ';
    character_states_[i]=0;
  }
  console_->consoleClearScreen();
}

uint32 Terminal::getNumRows() const
{
  return num_rows_;
}

uint32 Terminal::getNumColumns() const
{
  return num_columns_;
}

uint32 Terminal::setCharacter(uint32 row,uint32 column, uint8 character)
{
  characters_[column + row*num_columns_] = character;
  character_states_[column + row*num_columns_] = current_state_;
  return console_->consoleSetCharacter(row,column,character,current_state_);
}

void Terminal::setForegroundColor(Console::FOREGROUNDCOLORS const &color)
{
  // 4 bit set == 1+2+4+8, shifted by 0 bits
  uint8 mask = 15;
  current_state_ = current_state_ & ~mask;
  current_state_ |= color;  
}

void Terminal::setBackgroundColor(Console::BACKGROUNDCOLORS const &color)
{
  // 4 bit set == 1+2+4+8, shifted by 4 bits
  uint8 mask = 15<<4;
  uint8 col = color;
  current_state_ = current_state_ & ~mask;
  current_state_ |= col<<4;
}

void Terminal::scrollUp()
{
  uint32 i,k,runner;
  
  console_->consoleScrollUp();
  //console_->consoleClearScreen();
  runner = 0;
  for (i=0;i<num_rows_-1;++i)
  {
    for (k=0;k<num_columns_;++k)
    {
      characters_[runner] = characters_[runner+num_columns_];
      character_states_[runner] = character_states_[runner+num_columns_];
    //  console_->consoleSetCharacter(i,k,characters_[runner],character_states_[runner]);
      ++runner;
    }
  }
  for (i=0;i<num_columns_;++i)
  {
    characters_[runner] = 0;
    character_states_[runner] = 0;
    ++runner;
  }
}
