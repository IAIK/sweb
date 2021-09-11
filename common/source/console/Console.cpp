#include <mm/KernelMemoryManager.h>
#include "Console.h"
#include "Terminal.h"
#include "KeyboardManager.h"
#include "Scheduler.h"
#include "PageManager.h"
#include "backtrace.h"
#include "debug.h"
#include "ArchMulticore.h"

Console* main_console;

Console::Console(uint32, const char* name) : Thread(0, name, Thread::KERNEL_THREAD), console_lock_("Console::console_lock_"),
    set_active_lock_("Console::set_active_state_lock_"), locked_for_drawing_(0), active_terminal_(0)
{
  debug(CONSOLE, "Created console at [%p, %p)\n", this, (char*)this + sizeof(*this));
}

void Console::lockConsoleForDrawing()
{
  console_lock_.acquire(getCalledBefore(1));
  locked_for_drawing_ = 1;
}

void Console::unLockConsoleForDrawing()
{
  locked_for_drawing_ = 0;
  console_lock_.release(getCalledBefore(1));
}

void Console::handleKey(uint32 key)
{
  KeyboardManager * km = KeyboardManager::instance();

  uint32 terminal_selected = key - KEY_F1;

  if (km->isShift())
  {
    if (terminal_selected < getNumTerminals())
      setActiveTerminal(terminal_selected);

    return;
  }

// else...
  switch (key)
  {
    case KEY_F7:

      cpu_lapic.sendIPI(90, LAPIC::IPIDestination::OTHERS);
      Scheduler::instance()->printThreadList();
      Scheduler::instance()->printStackTraces();
      Scheduler::instance()->printLockingInformation();
      for(auto t : Scheduler::instance()->threads_)
      {
              kprintfd("Thread %p = %s\n", t, t->getName());
              ArchThreads::printThreadRegisters(t, true);
      }
      assert(false && "F7 pressed");
      break;
    case KEY_F8:
      debug_print_to_fb = !debug_print_to_fb;
      break;
    case KEY_F9:
      PageManager::instance()->printBitmap();
      kprintfd("Used kernel memory: %zu\n", KernelMemoryManager::instance()->getUsedKernelMemory(true));
      break;

    case KEY_F10:
      Scheduler::instance()->printLockingInformation();
      break;

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

Terminal *Console::getTerminal(uint32 term)
{
  return terminals_[term];
}

void Console::setActiveTerminal(uint32 term)
{
  set_active_lock_.acquire();

  Terminal *t = terminals_[active_terminal_];
  t->unSetAsActiveTerminal();

  t = terminals_[term];
  t->setAsActiveTerminal();
  active_terminal_ = term;

  set_active_lock_.release();
}

void Console::Run(void)
{
  KeyboardManager * km = KeyboardManager::instance();
  uint32 key;
  do
  {
    while (km->getKeyFromKbd(key))
    {
      if (isDisplayable(key))
      {
        key = terminals_[active_terminal_]->remap(key);
        terminals_[active_terminal_]->write(key);
        terminals_[active_terminal_]->putInBuffer(key);
      }
      else
      {
        handleKey(key);
      }
    }
    Scheduler::instance()->yield();
  } while (1);
}
bool Console::isDisplayable(uint32 key)
{
  return (((key & 127) >= ' ') || (key == '\n') || (key == '\b'));
}
