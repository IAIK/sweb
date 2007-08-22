//----------------------------------------------------------------------
//   $Id: Console.h,v 1.9 2005/09/15 17:51:13 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: Console.h,v $
//  Revision 1.8  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
//  Revision 1.7  2005/07/27 10:04:26  btittelbach
//  kprintf_nosleep and kprintfd_nosleep now works
//  Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//  Revision 1.6  2005/07/24 17:02:59  nomenquis
//  lots of changes for new console stuff
//
//  Revision 1.5  2005/04/23 20:08:26  nomenquis
//  updates
//
//  Revision 1.4  2005/04/23 18:13:26  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
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
#include "util/List.h"
#include "Mutex.h"

#include "Thread.h"

class Terminal;
class Console : public Thread
{
friend class Terminal;
friend class ConsoleManager;
  
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

  Console(uint32 num_terminals);
  virtual ~Console(){}
  uint32 getNumTerminals()const;
  Terminal *getActiveTerminal();
  Terminal *getTerminal(uint32 term);
  void setActiveTerminal(uint32 term);
  
  void lockConsoleForDrawing();
  void unLockConsoleForDrawing();
  
  virtual void Run();
  

    
  bool areLocksFree()
  {
    return (console_lock_.isFree() && console_lock_.isFree() && locked_for_drawing_==0);
  }


protected:
    
  virtual void consoleClearScreen()=0;
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)=0;
  virtual uint32 consoleGetNumRows() const=0;
  virtual uint32 consoleGetNumColumns() const=0;
  virtual void consoleScrollUp()=0;
  virtual void consoleSetForegroundColor(FOREGROUNDCOLORS const &color)=0;
  virtual void consoleSetBackgroundColor(BACKGROUNDCOLORS const &color)=0;

 
  List<Terminal *> terminals_;
  Mutex console_lock_;
  Mutex set_active_lock_;
  uint8 locked_for_drawing_;

  uint32 active_terminal_;

private:
  

};

extern Console* main_console;



#endif
