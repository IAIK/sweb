//----------------------------------------------------------------------
//   $Id: ConsoleManager.cpp,v 1.5 2005/04/23 20:08:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ConsoleManager.cpp,v $
//  Revision 1.4  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.3  2005/04/22 19:43:04  nomenquis
//   more poison added
//
//  Revision 1.2  2005/04/22 18:23:16  nomenquis
//  massive cleanups
//
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#include "console/ConsoleManager.h"
#include "mm/new.h"
#include "arch_panic.h"
#include "console/TextConsole.h"
#include "console/FrameBufferConsole.h"
#include "ArchCommon.h"

ConsoleManager *ConsoleManager::instance_ = 0;

void ConsoleManager::createConsoleManager(uint32 const &number_of_consoles_to_generate)
{
  instance_ = new ConsoleManager(number_of_consoles_to_generate);
}

ConsoleManager::ConsoleManager(uint32 const &number_of_consoles_to_generate)
{
  uint32 i;
  consoles_ = new Console*[number_of_consoles_to_generate];
  if (!consoles_)
  {
    arch_panic((uint8*)"Unable to allocate memory for consoles");
  }
  
  for (i=0;i<number_of_consoles_to_generate;++i)
  {
    if (ArchCommon::haveVESAConsole())
      consoles_[i] = new FrameBufferConsole();
    else
      consoles_[i] = new TextConsole();
    
    if (!consoles_[i])
    {
      arch_panic((uint8*)"Unable to allocate memory for console");
    }
  }
  number_of_consoles_ = number_of_consoles_to_generate;
  
  if (number_of_consoles_)
  {
    //consoles_[0]->setAsCurrent();
    active_console_ = 0;
  }
}

uint32 ConsoleManager::setActiveConsole(uint32 const &console_number)
{
  uint32 ret = 0;
  if (console_number >= number_of_consoles_)
    return 1;
  
  if (active_console_ == console_number)
    return 0;
  
//  if (ret = consoles_[active_console_]->unsetAsCurrent())
//    return ret;
  
 // ret = consoles_[console_number]->setAsCurrent();
  active_console_ = console_number;
  
  return ret;
}

Console *ConsoleManager::getActiveConsole() const
{
  return consoles_[active_console_];
}
