/**
 * @file Console.cpp
 */
#include "Console.h"
#include "Terminal.h"

Console* main_console=0;

Console::Console ( uint32 ) : Thread()
{
  name_ = "ConsoleThread";
}

void Console::lockConsoleForDrawing()
{
  console_lock_.acquire();
  locked_for_drawing_ = 1;
}

void Console::unLockConsoleForDrawing()
{
  locked_for_drawing_ = 0;
  console_lock_.release();
}

uint32 Console::getNumTerminals() const
{
  return terminals_.size();
}

Terminal *Console::getActiveTerminal()
{
  set_active_lock_.acquire();
  Terminal * act = 0;
  console_lock_.acquire();
  act = terminals_[active_terminal_];
  console_lock_.release();
  set_active_lock_.release();
  return act;
}

Terminal *Console::getTerminal ( uint32 term )
{
  return terminals_[term];
}

void Console::setActiveTerminal ( uint32 term )
{
  // most likely this code will produce a dead lock
  set_active_lock_.acquire();

  Terminal *t = terminals_[active_terminal_];
  t->unSetAsActiveTerminal();

  t = terminals_[term];
  t->setAsActiveTerminal();
  active_terminal_ = term;

  set_active_lock_.release();
}

void Console::Run ( void )
{
  // we do not run
}
