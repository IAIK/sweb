//----------------------------------------------------------------------
//   $Id: XenConsole.cpp,v 1.2 2005/10/08 10:42:33 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: XenConsole.cpp,v $
//  Revision 1.1  2005/09/28 16:35:43  nightcreature
//  main.cpp: added XenConsole (partly implemented but works) to replace TextConsole
//  in xenbuild, first batch of fixes in xen part
//
//----------------------------------------------------------------------


#include "XenConsole.h"
#include "Terminal.h"
#include "ArchCommon.h"
#include "panic.h"

#include "Scheduler.h"

#include "arch_keyboard_manager.h"
//#include "xenprintf.h"

extern "C" int xenprintf(const char *fmt, ...);
extern "C" int xenvprintf(const char *fmt, char *ap);
extern "C" int xensprintf(char *buf, const char *cfmt, ...);
extern "C" int xenvsprintf(char *buf, const char *cfmt, char *ap);

XenConsole::XenConsole(uint32 num_terminals):Console(num_terminals)
{
  uint32 i;
  for (i=0;i<num_terminals;++i)
  {
    Terminal *term = new Terminal("",this,consoleGetNumColumns(),consoleGetNumRows());
    terminals_.pushBack(term);
  }
  active_terminal_ = 0;
  name_ = "TxTConsoleThrd";
}

uint32 XenConsole::consoleGetNumRows()const
{
  return 25;
}

uint32 XenConsole::consoleGetNumColumns()const
{
  return 80;
}

void XenConsole::consoleClearScreen()
{
//  if (unlikely(!locked_for_drawing_))
//    panic("Console not locked for drawing, this is REALLY bad");
  
//   char *fb = (char*)ArchCommon::getFBPtr();
//   uint32 i;
//   for (i=0;i<consoleGetNumRows()*consoleGetNumColumns()*2;++i)
//   {
//     fb[i]=0;
//   }
  const char *line = "                                                                                ";
  uint32 i;
  for(i=0;i<25;++i)
    xenprintf("%s\n",line);
}

uint32 XenConsole::consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state)
{
//  if (unlikely(!locked_for_drawing_))
//    panic("Console not locked for drawing, this is REALLY bad");
  
//   char *fb = (char*)ArchCommon::getFBPtr();
//   fb[(column + row*consoleGetNumColumns())*2] = character;
//   fb[(column + row*consoleGetNumColumns())*2+1] = state;

  xenprintf("%c",character);
  
  return 0;
}

void XenConsole::consoleScrollUp()
{
//  if (unlikely(!locked_for_drawing_))
//    panic("Console not locked for drawing, this is REALLY bad");
  
//   pointer fb = ArchCommon::getFBPtr();
//   ArchCommon::memcpy(fb, fb+(consoleGetNumColumns()*2),
//     (consoleGetNumRows()-1)*consoleGetNumColumns()*2);
//   ArchCommon::bzero(fb+((consoleGetNumRows()-1)*consoleGetNumColumns()*2),consoleGetNumColumns()*2);
  xenprintf("\n");
}



void XenConsole::consoleSetForegroundColor(FOREGROUNDCOLORS const &color)
{
}

void XenConsole::consoleSetBackgroundColor(BACKGROUNDCOLORS const &color)
{
}

void XenConsole::Run( void )
{
  KeyboardManager * km = KeyboardManager::getInstance();
  uint32 key=(uint32)-1;
  do 
  {
    while(km->getKeyFromKbd(key))
      if( isDisplayable( key ) )
      {
        key = remap( key );
        terminals_[active_terminal_]->write( key );
        terminals_[active_terminal_]->putInBuffer( key );
      }
      else
        handleKey( key );
    Scheduler::instance()->yield();
    if (key==(uint32)-1)  
      km->emptyKbdBuffer();
    //we assume above here, that irq1 has never been fired, presumeably because
    //something was in the kbd buffer bevore irq1 got enabled
  }
  while(1); // until the end of time
}

void XenConsole::handleKey( uint32 key )
{
  KeyboardManager * km = KeyboardManager::getInstance();
  
  uint32 terminal_selected = (key - KeyboardManager::KEY_F1);
  
  if( terminal_selected < getNumTerminals() && km->isShift() )
  {
    setActiveTerminal(terminal_selected);
    return;
  }
  
  if (terminal_selected == 11)
    Scheduler::instance()->printThreadList();
  
  if( key == '\b' )
    terminals_[active_terminal_]->backspace();
  
  return;
}

bool XenConsole::isDisplayable( uint32 key )
{
  return ( ( (key & 127) >= ' ' ) || (key == '\n') || (key == '\b') );
}

bool XenConsole::isLetter( uint32 key )
{
  return ( ( key >= 'a' ) && (key <= 'z') );
}

bool XenConsole::isNumber( uint32 key )
{
  return ( ( key >= '0' ) && (key <= '9') );
}

uint32 XenConsole::remap( uint32 key )
{

  /// TODO: Move this function in terminal and
  ///       implement lookup tables for various
  ///       keyboard layouts
  
  uint32 number_table[] = { ')', '!', '@', '#', '$', 
                            '%', '^', '&', '*', '('  };
  
  KeyboardManager * km = KeyboardManager::getInstance();
  
  if ( isLetter( key ) )
  {
    bool shifted = km->isShift() ^ km->isCaps();
    
    if( shifted )
      key &= ~0x20;  
  }
  
  if ( isNumber( key ) )
  {
    if( km->isShift() )
        key = number_table[ key - '0' ];
    
  }
  
  return key;
}
