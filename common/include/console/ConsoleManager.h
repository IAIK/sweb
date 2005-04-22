//----------------------------------------------------------------------
//   $Id: ConsoleManager.h,v 1.1 2005/04/22 17:21:40 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef __CONSOLE_MANAGER_H__
#define __CONSOLE_MANAGER_H__

#include "types.h"
#include "Console.h"

class ConsoleManager
{
public:

  ConsoleManager(uint32 const &number_of_consoles_to_generate);

  uint32 setActiveConsole(uint32 const &console_number);

private:
  
  uint32 number_of_consoles_;
  Console **consoles_;
  uint32 active_console_;

};



#endif
