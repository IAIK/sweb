//----------------------------------------------------------------------
//  $Id: Console.cpp,v 1.2 2005/07/24 17:02:59 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Console.cpp,v $
//  Revision 1.1  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//----------------------------------------------------------------------

#include "Console.h"
#include "Terminal.h"

Console* main_console=0;

Console::Console(uint32)
{

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

uint32 Console::getNumTerminals()const
{
  return terminals_.size();
}

Terminal *Console::getActiveTerminal()
{
  // ok, this one is tricky, 
  set_active_lock_.acquire();
  Terminal * act = 0;
  console_lock_.acquire();
  act = terminals_[active_terminal_];
  console_lock_.release();
  set_active_lock_.release();
  return act;  
}

Terminal *Console::getTerminal(uint32 term)
{
  return terminals_[term];  
}

void Console::setActiveTerminal(uint32 term)
{
  // this one is a bitch
  // no, really
  // most likely this code will produce a dead lock
  set_active_lock_.acquire();
  
  Terminal *t = terminals_[active_terminal_];
  t->unSetAsActiveTerminal();
  
  t = terminals_[term];
  t->setAsActiveTerminal();
  active_terminal_ = term;
  
  set_active_lock_.release();
}
