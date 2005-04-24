/**
 * $Id: main.cpp,v 1.20 2005/04/24 20:32:14 btittelbach Exp $
 *
 * $Log: main.cpp,v $
 * Revision 1.19  2005/04/24 18:58:45  btittelbach
 * kprintf bugfix
 *
 * Revision 1.18  2005/04/24 16:58:04  nomenquis
 * ultra hack threading
 *
 * Revision 1.16  2005/04/24 10:06:09  nomenquis
 * commit to compile on different machine
 *
 * Revision 1.15  2005/04/23 20:32:30  nomenquis
 * timer interrupt works
 *
 * Revision 1.14  2005/04/23 20:08:26  nomenquis
 * updates
 *
 * Revision 1.13  2005/04/23 17:35:03  nomenquis
 * fixed buggy memory manager
 * (giving out the same memory several times is no good idea)
 *
 * Revision 1.12  2005/04/23 16:03:40  btittelbach
 * kmm testen im main
 *
 * Revision 1.11  2005/04/23 15:58:32  nomenquis
 * lots of new stuff
 *
 * Revision 1.10  2005/04/23 12:52:26  nomenquis
 * fixes
 *
 * Revision 1.9  2005/04/23 12:43:09  nomenquis
 * working page manager
 *
 * Revision 1.8  2005/04/22 20:14:25  nomenquis
 * fix for crappy old gcc versions
 *
 * Revision 1.7  2005/04/22 19:43:04  nomenquis
 *  more poison added
 *
 * Revision 1.6  2005/04/22 18:23:16  nomenquis
 * massive cleanups
 *
 * Revision 1.5  2005/04/22 17:21:41  nomenquis
 * added TONS of stuff, changed ZILLIONS of things
 *
 * Revision 1.4  2005/04/21 21:31:24  nomenquis
 * added lfb support, also we now use a different grub version
 * we also now read in the grub multiboot version
 *
 * Revision 1.3  2005/04/20 08:06:18  nomenquis
 * the overloard (thats me) managed to get paging with 4m pages to work.
 * kernel is now at 2g +1 and writes something to the fb
 * w00t!
 *
 * Revision 1.2  2005/04/20 06:39:11  nomenquis
 * merged makefile, also removed install from default target since it does not work
 *
 * Revision 1.1  2005/04/12 18:42:51  nomenquis
 * changed a zillion of iles
 *
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */
 
#include "types.h"
#include "multiboot.h"
#include "arch_panic.h"
#include "paging-definitions.h"
#include "console/ConsoleManager.h"
#include "console/kprintf.h"
#include "mm/new.h"
#include "mm/PageManager.h"
#include "mm/KernelMemoryManager.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"

extern void* kernel_end_address;

extern "C" void startup();

void fun1()
{
  
  while (1)
    ConsoleManager::instance()->getActiveConsole()->writeString((uint8*)"Thread 1\n");
}

void fun2()
{
  while (1)
    ConsoleManager::instance()->getActiveConsole()->writeString((uint8*)"Thread 2\n");
}

void startup()
{
  
  
  pointer start_address = (pointer)&kernel_end_address;
  pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U);
  start_address = PageManager::createPageManager(start_address);
  KernelMemoryManager::createMemoryManager(start_address,end_address);
  
  ConsoleManager::createConsoleManager(1);

  Console *console = ConsoleManager::instance()->getActiveConsole();

  console->setBackgroundColor(Console::BG_BLACK);
  console->setForegroundColor(Console::FG_GREEN);
/*
  console->writeString((uint8 const*)"Blabb\n");  
  console->writeString((uint8 const*)"Blubb sagte die Katze und frasz den Hund\n");
  console->writeString((uint8 const*)"Noch ne Zeile\n");
  console->writeString((uint8 const*)"Und jetzt ne leere\n\n");
  console->writeString((uint8 const*)"Gruen rackete autobus\n");
  console->writeString((uint8 const*)"LAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNNGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRR STRING\n");
*/
  // initialise everything, meaning that we'll have a dummy 
  // thread info for the main thread
  //ArchThreads::initialise();
  
  kprintf("test %d %o %u %d noch ein String: %s und was hexadezimales %x\nein padding:\n%10d\n%10d\nzero padding:\n%+10d\n%010d\n", 1, 2, -3,-4, "'hallo'",16,200,300,400,500);
  
  ArchThreads::initDemo((pointer)&fun1, (pointer)&fun2);

  ArchThreads::initialise();
  ArchInterrupts::initialise();
  ArchInterrupts::enableTimer();
  ArchInterrupts::enableInterrupts();


//  thread1 = make_thread((pointer)&printbla);
//  thread2 = make_thread((pointer)&printblubb);
  
  for (;;);
}
