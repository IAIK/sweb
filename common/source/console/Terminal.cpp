//----------------------------------------------------------------------
//  $Id: Terminal.cpp,v 1.12 2005/09/27 21:24:43 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Terminal.cpp,v $
//  Revision 1.11  2005/09/21 22:31:37  btittelbach
//  consider \r
//
//  Revision 1.10  2005/09/21 21:29:45  btittelbach
//  make kernel readline do less, as its suppossed to
//
//  Revision 1.9  2005/09/16 15:47:41  btittelbach
//  +even more KeyboardInput Bugfixes
//  +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//  +Thread now remembers its Terminal
//  +Syscalls are USEABLE !! :-) IF=1 !!
//  +Syscalls can block now ! ;-) Waiting for Input...
//  +more other Bugfixes
//
//  Revision 1.8  2005/09/16 12:47:41  btittelbach
//  Second PatchThursday:
//  +KeyboardInput SyncStructure Rewrite
//  +added RingBuffer
//  +debugged FiFoDRBOSS (even though now obsolete)
//  +improved FiFo
//  +more debugging
//  Added Files:
//   	common/include/ipc/RingBuffer.h
//
//  Revision 1.7  2005/09/16 00:54:13  btittelbach
//  Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
//  Revision 1.6  2005/09/15 18:47:07  btittelbach
//  FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//  IdleThread now uses hlt instead of yield.
//
//  Revision 1.5  2005/09/15 17:51:13  nelles
//
//
//   Massive update. Like PatchThursday.
//   Keyboard is now available.
//   Each Terminal has a buffer attached to it and threads should read the buffer
//   of the attached terminal. See TestingThreads.h in common/include/kernel for
//   example of how to do it.
//   Switching of the terminals is done with the SHFT+F-keys. (CTRL+Fkeys gets
//   eaten by X on my machine and does not reach Bochs).
//   Lot of smaller modifications, to FiFo, Mutex etc.
//
//   Committing in .
//
//   Modified Files:
//   	arch/x86/source/InterruptUtils.cpp
//   	common/include/console/Console.h
//   	common/include/console/Terminal.h
//   	common/include/console/TextConsole.h common/include/ipc/FiFo.h
//   	common/include/ipc/FiFoDRBOSS.h common/include/kernel/Mutex.h
//   	common/source/console/Console.cpp
//   	common/source/console/Makefile
//   	common/source/console/Terminal.cpp
//   	common/source/console/TextConsole.cpp
//   	common/source/kernel/Condition.cpp
//   	common/source/kernel/Mutex.cpp
//   	common/source/kernel/Scheduler.cpp
//   	common/source/kernel/Thread.cpp common/source/kernel/main.cpp
//   Added Files:
//   	arch/x86/include/arch_keyboard_manager.h
//   	arch/x86/source/arch_keyboard_manager.cpp
//
//  Revision 1.4  2005/07/24 17:02:59  nomenquis
//  lots of changes for new console stuff
//
//  Revision 1.3  2005/04/26 16:08:59  nomenquis
//  updates
//
//  Revision 1.2  2005/04/23 18:13:27  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
//  Revision 1.1  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//----------------------------------------------------------------------

#include "Terminal.h"
#include "Console.h"

#include "arch_keyboard_manager.h"

#include "kprintf.h"

Terminal::Terminal(Console *console, uint32 num_columns, uint32 num_rows):
  console_(console), num_columns_(num_columns), num_rows_(num_rows), len_(num_rows * num_columns),
  current_column_(0), current_state_(0x93), active_(0)
{
  characters_ = new uint8[len_];
  character_states_ = new uint8[len_];
  
  uint32 i;
  for (i=0;i<len_;++i)
  {
    characters_[i]=' ';
    character_states_[i]=0;
  }
    
  //clearScreen();
  
  terminal_buffer_ = new FiFo< uint32 >( TERMINAL_BUFFER_SIZE , FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD );  
}
  
void Terminal::clearBuffer()
{
  terminal_buffer_->clear();
}

void Terminal::putInBuffer( uint32 what )
{
  terminal_buffer_->put( what );
}

char Terminal::read()
{
  return (char) terminal_buffer_->get();
}

void Terminal::backspace( void )
{
  if( terminal_buffer_->countElementsAhead() )
    terminal_buffer_->get();
  
  if( current_column_ )
  {
    --current_column_;
    setCharacter(num_rows_-1, current_column_, ' ');
  }
}

uint32 Terminal::readLine( char *line, uint32 size )
{
  uint32 cchar;
  uint32 counter = 0;
  if (size < 1)
    return 0;
  do {
    cchar = terminal_buffer_->get();
    
    if( cchar == '\b' )
    {
      if (counter>0)
        counter--;
    }
    else
      line[counter++] = (char) cchar;
  }
  while( cchar != '\n' && cchar != '\r' && counter < size );

  if(size-counter)
    line[counter]= '\0';
  return counter;
}

uint32 Terminal::readLineNoBlock( char *line, uint32 size )
{
  uint32 cchar;
  uint32 counter = 0;
  if (size < 1)
    return 0;
  while (terminal_buffer_->countElementsAhead())
  {
    cchar = terminal_buffer_->get();
    
    if( cchar == '\b' )
    {
      if (counter>0)
        counter--;
    }
    else
      line[counter++] = (char) cchar;
    
    if ( cchar != '\n' && cchar != '\r' && counter < (size-1) );
   }
  
   line[counter] = '\0';
   
   return counter;
}

void Terminal::writeInternal(char character)
{
  if (character == '\n' || character == '\r')
  {
    scrollUp();
    current_column_ = 0;
  }
  else if ( character == '\b' )
    backspace();
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

void Terminal::write(char character)
{
  MutexLock lock(mutex_);
  console_->lockConsoleForDrawing();
  writeInternal(character);
  console_->unLockConsoleForDrawing();  

}
void Terminal::writeString(char const *string)
{
  MutexLock lock(mutex_);
  console_->lockConsoleForDrawing();
  while (string && *string)
  {
    writeInternal(*string);
    ++string;
  }
  console_->unLockConsoleForDrawing();  
}

void Terminal::writeBuffer(char const *buffer, size_t len)
{
  writeString("Sorry, buffer writing not implemented yet\n");
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
  if (active_)
    console_->consoleSetCharacter(row,column,character,current_state_);
  
  return 0;
}

void Terminal::setForegroundColor(Console::FOREGROUNDCOLORS const &color)
{
  MutexLock lock(mutex_);
  // 4 bit set == 1+2+4+8, shifted by 0 bits
  uint8 mask = 15;
  current_state_ = current_state_ & ~mask;
  current_state_ |= color;  
}

void Terminal::setBackgroundColor(Console::BACKGROUNDCOLORS const &color)
{
  MutexLock lock(mutex_);
  // 4 bit set == 1+2+4+8, shifted by 4 bits
  uint8 mask = 15<<4;
  uint8 col = color;
  current_state_ = current_state_ & ~mask;
  current_state_ |= col<<4;
}

void Terminal::scrollUp()
{
  uint32 i,k,runner;
  
  runner = 0;
  for (i=0;i<num_rows_-1;++i)
  {
    for (k=0;k<num_columns_;++k)
    {
      characters_[runner] = characters_[runner+num_columns_];
      character_states_[runner] = character_states_[runner+num_columns_];
      ++runner;
    }
  }
  for (i=0;i<num_columns_;++i)
  {
    characters_[runner] = 0;
    character_states_[runner] = 0;
    ++runner;
  }
  if (active_)
      console_->consoleScrollUp();

}
void Terminal::fullRedraw()
{
  console_->lockConsoleForDrawing();
  uint32 i,k;
  uint32 runner=0;
  for (i=0;i<num_rows_;++i)
  {
    for (k=0;k<num_columns_;++k)
    {
      console_->consoleSetCharacter(i,k,characters_[runner],character_states_[runner]);
      ++runner;
    }
  }
  
  console_->unLockConsoleForDrawing();
}

void Terminal::setAsActiveTerminal()
{
  MutexLock lock(mutex_);
  active_ = 1;
  fullRedraw();
}

void Terminal::unSetAsActiveTerminal()
{
  MutexLock lock(mutex_);
  active_ = 0;
}
