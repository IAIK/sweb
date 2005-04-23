//----------------------------------------------------------------------
//   $Id: ConsoleManager.h,v 1.3 2005/04/23 20:08:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ConsoleManager.h,v $
//  Revision 1.2  2005/04/23 15:58:32  nomenquis
//  lots of new stuff
//
//  Revision 1.1  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#ifndef __CONSOLE_MANAGER_H__
#define __CONSOLE_MANAGER_H__

#include "types.h"
#include "Console.h"

class ConsoleManager
{
public:

  static void createConsoleManager(uint32 const &number_of_consoles_to_generate);
  static ConsoleManager *instance(){return instance_;}


  uint32 setActiveConsole(uint32 const &console_number);
  Console *getActiveConsole() const;
private:
  
  ConsoleManager(uint32 const &number_of_consoles_to_generate);

  uint32 number_of_consoles_;
  Console **consoles_;
  uint32 active_console_;
  
  static ConsoleManager *instance_;

};



#endif
