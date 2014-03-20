/**
 * @file Console.cpp
 */
#include "Console.h"
#include "Terminal.h"
#include "arch_keyboard_manager.h"

Console* main_console=0;

Console::Console ( uint32 ) : Thread("ConsoleThread"), console_lock_("Console::console_lock_"), set_active_lock_("Console::set_active_state_lock_"), locked_for_drawing_(0), active_terminal_(0)
{
}

Console::Console ( uint32, const char* name ) : Thread(name), console_lock_("Console::console_lock_"), set_active_lock_("Console::set_active_state_lock_"), locked_for_drawing_(0), active_terminal_(0)
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

void Console::handleKey ( uint32 key )
{
  KeyboardManager * km = KeyboardManager::getInstance();

  uint32 terminal_selected = key - KEY_F1;

  if (km->isShift())
  {
    if (terminal_selected < getNumTerminals())
      setActiveTerminal (terminal_selected);

    return;
  }

// else...
  switch (key)
  {
    case KEY_F11:
      Scheduler::instance()->printStackTraces();
       break;

    case KEY_F12:
      Scheduler::instance()->printThreadList();
      break;

    case '\b':
      terminals_[active_terminal_]->backspace();
      break;
  }
}

uint32 Console::getNumTerminals() const
{
  return terminals_.size();
}

Terminal *Console::getActiveTerminal()
{
  return terminals_[active_terminal_]; // why not locked? integer read is consistent anyway
}

Terminal *Console::getTerminal ( uint32 term )
{
  return terminals_[term];
}

void Console::setActiveTerminal ( uint32 term )
{
  set_active_lock_.acquire();

  Terminal *t = terminals_[active_terminal_];
  t->unSetAsActiveTerminal();

  t = terminals_[term];
  t->setAsActiveTerminal();
  active_terminal_ = term;

  set_active_lock_.release();
}
