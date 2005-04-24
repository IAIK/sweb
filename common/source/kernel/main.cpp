/**
 * $Id: main.cpp,v 1.16 2005/04/24 10:06:09 nomenquis Exp $
 *
 * $Log: main.cpp,v $
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
#include "mm/new.h"
#include "mm/PageManager.h"
#include "mm/KernelMemoryManager.h"
#include "ArchInterrupts.h"

extern void* kernel_end_address;

extern "C"
{
  static uint32 TickTack()
  {
    ConsoleManager::instance()->getActiveConsole()->writeString((uint8*)"Tick\n");
    return 0;
  }
  
  
typedef struct ArchThreadInfo
{
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  dpl;       // 64
  uint32  esp0;      // 68
  uint32  ss0;       // 72
  uint32  cr3;       // 76
  uint32  fpu[27];   // 80
};

typedef struct ArchThread
{
  ArchThreadInfo *thread_info;
  uint8 *stack;
  
};

ArchThread* make_thread(pointer eip)
{
  ArchThread * bla = new ArchThread();
  // bla->thread_info = new ArchThreadInfo();
 // bla->stack = new uint8[4000];
  
  ArchThreadInfo *info = bla->thread_info;
typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 type;
    uint8 limitH;
    uint8 baseH;
} SegDesc;

#define GDT_ENTRY_SELECTOR(n) (n * sizeof(SegDesc))
#define KERNEL_CS GDT_ENTRY_SELECTOR(1)
#define KERNEL_DS GDT_ENTRY_SELECTOR(2)
#define KERNEL_SS GDT_ENTRY_SELECTOR(3)
#define USER_CS   GDT_ENTRY_SELECTOR(5) | DPL_USER
#define USER_DS   GDT_ENTRY_SELECTOR(6) | DPL_USER
#define USER_SS   GDT_ENTRY_SELECTOR(7) | DPL_USER

  info->cs      = KERNEL_CS;
  info->ds      = KERNEL_DS;
  info->es      = KERNEL_DS;
  info->ss      = KERNEL_SS;
  info->eflags  = 0x200;
  info->eax     = 0;
  info->ecx     = 0;
  info->edx     = 0;
  info->ebx     = 0;
  info->esi     = 0;
  info->edi     = 0;
  info->dpl     = 0;
  info->esp     = (pointer)bla->stack;
  info->ebp     = (pointer)bla->stack;
  info->eip     = eip;
  
  extern uint32 kernel_page_directory_start;
  info->cr3     = ((pointer)&kernel_page_directory_start)-2*1024*1024*1024;

 /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;

  
  
  
}

ArchThread *thread1;
ArchThread *thread2;
ArchThread *current;

void printbla()
{
  Console *console = ConsoleManager::instance()->getActiveConsole();
  console->writeString((uint8 const*)"Bla\n");  
}
void printblubb()
{
  Console *console = ConsoleManager::instance()->getActiveConsole();
  console->writeString((uint8 const*)"Blubb\n");  
}

	void startup()
	{
    pointer start_address = (pointer)&kernel_end_address;
    pointer end_address = (pointer)(1024*1024*1024*2 + 1024*1024*4);
    start_address = PageManager::createPageManager(start_address);
    KernelMemoryManager::createMemoryManager(start_address,end_address);
    ConsoleManager::createConsoleManager(1);

    Console *console = ConsoleManager::instance()->getActiveConsole();

    console->setBackgroundColor(Console::BG_BLACK);
    console->setForegroundColor(Console::FG_GREEN);

    console->writeString((uint8 const*)"Blabb\n");  
    console->writeString((uint8 const*)"Blubb sagte die Katze und frasz den Hund\n");
    console->writeString((uint8 const*)"Noch ne Zeile\n");
    console->writeString((uint8 const*)"Und jetzt ne leere\n\n");
    console->writeString((uint8 const*)"Gruen rackete autobus\n");
    console->writeString((uint8 const*)"LAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNNGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRR STRING\n");

    ArchInterrupts::initialise();
    ArchInterrupts::setTimerHandler(&TickTack);
    ArchInterrupts::enableTimer();
    ArchInterrupts::enableInterrupts();

  //  thread1 = make_thread((pointer)&printbla);
  //  thread2 = make_thread((pointer)&printblubb);
    
    for (;;);
	}
  	  
}
