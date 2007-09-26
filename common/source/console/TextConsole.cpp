/**
 * @file TextConsole.cpp
 */

#include "TextConsole.h"
#include "Terminal.h"
#include "ArchCommon.h"
#include "panic.h"

#include "Scheduler.h"

#include "arch_keyboard_manager.h"
#include "kprintf.h"

TextConsole::TextConsole ( uint32 num_terminals ) :Console ( num_terminals )
{
  uint32 i, j = 10, log = 1, k = 0, l = 0;

  while ( num_terminals / j )
  {
    j *= 10;
    log++;
  }

  char *term_name = new char[ log + 5 ];
  term_name[ 0 ] = 't';
  term_name[ 1 ] = 'e';
  term_name[ 2 ] = 'r';
  term_name[ 3 ] = 'm';
  term_name[ log + 4 ] = 0;

  for ( i=0;i<num_terminals;++i )
  {
    uint32 cterm = i;
    for ( l = 4, k = j/10; k > 0 ; k /= 10, l++ )
    {
      term_name[ l ] = cterm/k + '0';
      cterm -= ( ( cterm/k ) * k );
    }

    Terminal *term = new Terminal ( term_name, this,consoleGetNumColumns(),consoleGetNumRows() );
    terminals_.pushBack ( term );
  }

  delete term_name;

  active_terminal_ = 0;
  name_ = "TxTConsoleThrd";
}

uint32 TextConsole::consoleGetNumRows() const
{
  return 25;
}

uint32 TextConsole::consoleGetNumColumns() const
{
  return 80;
}

void TextConsole::consoleClearScreen()
{
  char *fb = ( char* ) ArchCommon::getFBPtr();
  uint32 i;
  for ( i=0;i<consoleGetNumRows() *consoleGetNumColumns() *2;++i )
  {
    fb[i]=0;
  }
}

uint32 TextConsole::consoleSetCharacter ( uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state )
{
  char *fb = ( char* ) ArchCommon::getFBPtr();
  fb[ ( column + row*consoleGetNumColumns() ) *2] = character;
  fb[ ( column + row*consoleGetNumColumns() ) *2+1] = state;

  return 0;
}

void TextConsole::consoleScrollUp()
{
  pointer fb = ArchCommon::getFBPtr();
  ArchCommon::memcpy ( fb, fb+ ( consoleGetNumColumns() *2 ),
                       ( consoleGetNumRows()-1 ) *consoleGetNumColumns() *2 );
  ArchCommon::bzero ( fb+ ( ( consoleGetNumRows()-1 ) *consoleGetNumColumns() *2 ),consoleGetNumColumns() *2 );
}



void TextConsole::consoleSetForegroundColor ( FOREGROUNDCOLORS const &color )
{
  if ( color )
    return;
}

void TextConsole::consoleSetBackgroundColor ( BACKGROUNDCOLORS const &color )
{
  if ( color )
    return;
}

void TextConsole::Run ( void )
{
  KeyboardManager * km = KeyboardManager::getInstance();
  uint32 key= ( uint32 )-1;
  do
  {
    while ( km->getKeyFromKbd ( key ) )
      if ( isDisplayable ( key ) )
      {
        key = terminals_[active_terminal_]->remap ( key );
        terminals_[active_terminal_]->write ( key );
        terminals_[active_terminal_]->putInBuffer ( key );
      }
      else
        handleKey ( key );
    Scheduler::instance()->yield();
    if ( key== ( uint32 )-1 )
      km->emptyKbdBuffer();
    //we assume above here, that irq1 has never been fired, presumeably because
    //something was in the kbd buffer bevore irq1 got enabled
  }
  while ( 1 ); // until the end of time
}

void TextConsole::handleKey ( uint32 key )
{
  KeyboardManager * km = KeyboardManager::getInstance();

  uint32 terminal_selected = ( key - KeyboardManager::KEY_F1 );

  if ( terminal_selected < getNumTerminals() && km->isShift() )
  {
    setActiveTerminal ( terminal_selected );
    return;
  }

  if ( terminal_selected == 11 )
    Scheduler::instance()->printThreadList();

  if ( key == '\b' )
    terminals_[active_terminal_]->backspace();

  return;
}

bool TextConsole::isDisplayable ( uint32 key )
{
  return ( ( ( key & 127 ) >= ' ' ) || ( key == '\n' ) || ( key == '\b' ) );
}
