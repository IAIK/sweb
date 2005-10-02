//----------------------------------------------------------------------
//  $Id: Terminal.h,v 1.10 2005/10/02 12:27:55 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: Terminal.h,v $
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
//  Revision 1.6  2005/09/15 18:47:06  btittelbach
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
//  Revision 1.4  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
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
#include "Thread.h"
#include "FiFo.h"

#include "chardev.h"

class Console;
  
class Terminal : public CharacterDevice
{
friend class Console;
public:

  static uint32 const TERMINAL_BUFFER_SIZE = 256;

  Terminal(char *name, Console *console, uint32 num_columns, uint32 num_rows);
  
  void write(char character);
  void writeString(char const *string);
  void writeBuffer(char const *buffer, size_t len);
  
  virtual int32 writeData (int32 offset, int32 size, const char*buffer);

  void setForegroundColor(Console::FOREGROUNDCOLORS const &color);
  void setBackgroundColor(Console::BACKGROUNDCOLORS const &color);

  void writeInternal(char character);
  
  char read();
  uint32 readLine(char *, uint32);
  uint32 readLineNoBlock(char *, uint32);
  
  void clearBuffer();

  void putInBuffer( uint32 key );
  
  void backspace( void );

  bool isLockFree()
  {
    return mutex_.isFree();
  }

protected:
  
  void setAsActiveTerminal();
  void unSetAsActiveTerminal();

private:

  void handleKey( uint32 key );
  
  void clearScreen();
  void fullRedraw();
  uint32 getNumRows() const;
  uint32 getNumColumns() const;

  uint32 setCharacter(uint32 row,uint32 column, uint8 character);
  
  void processInBuffer( void ) {};
  void processOutBuffer( void ) {};  

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
