//----------------------------------------------------------------------
//   $Id: ConsoleManager.cpp,v 1.2 2005/04/22 18:23:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ConsoleManager.cpp,v $
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

ConsoleManager::ConsoleManager(uint32 const &number_of_consoles_to_generate)
{
  uint32 i;
  consoles_ = new Console*[number_of_consoles_to_generate];
  if (!consoles_)
  {
    arch_panic("Unable to allocate memory for consoles");
  }
  
  for (i=0;i<number_of_consoles_to_generate;++i)
  {
    if (ArchCommon::haveVESAConsole())
      consoles_[i] = new FrameBufferConsole();
    else
      consoles_[i] = new TextConsole();
    
    if (!consoles_[i])
    {
      arch_panic("Unable to allocate memory for console");
    }
  }
  number_of_consoles_ = number_of_consoles_to_generate;
  
  if (number_of_consoles_)
  {
    consoles_[0]->setAsCurrent();
    active_console_ = 0;
  }
}

uint32 ConsoleManager::setActiveConsole(uint32 const &console_number)
{
  int ret = 0;
  if (console_number >= number_of_consoles_)
    return 1;
  
  if (active_console_ == console_number)
    return 0;
  
  if (ret = consoles_[active_console_]->unsetAsCurrent())
    return ret;
  
  ret = consoles_[console_number]->setAsCurrent();
  active_console_ = console_number;
  
  return ret;
}
