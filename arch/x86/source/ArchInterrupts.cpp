//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.16 2005/11/20 21:18:08 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.15  2005/10/26 10:25:22  nelles
//
//  Small patch
//
//   	source/ArchInterrupts.cpp source/arch_bd_ata_driver.cpp
//
//  Revision 1.14  2005/09/18 20:25:05  nelles
//
//
//  Block devices update.
//  See BDRequest and BDManager on how to use this.
//  Currently ATADriver is functional. The driver tries to detect if IRQ
//  mode is available and adjusts the mode of operation. Currently PIO
//  modes with IRQ or without it are supported.
//
//  TODO:
//  - add block PIO mode to read or write multiple sectors within one IRQ
//  - add DMA and UDMA mode :)
//
//
//   Committing in .
//
//   Modified Files:
//   	arch/common/include/ArchInterrupts.h
//   	arch/x86/source/ArchInterrupts.cpp
//   	arch/x86/source/InterruptUtils.cpp
//   	common/include/kernel/TestingThreads.h
//   	common/source/kernel/Makefile
//   	common/source/kernel/Scheduler.cpp
//   	common/source/kernel/main.cpp utils/bochs/bochsrc
//   Added Files:
//   	arch/x86/include/arch_bd_ata_driver.h
//   	arch/x86/include/arch_bd_driver.h
//   	arch/x86/include/arch_bd_ide_driver.h
//   	arch/x86/include/arch_bd_io.h
//  	arch/x86/include/arch_bd_manager.h
//   	arch/x86/include/arch_bd_request.h
//   	arch/x86/include/arch_bd_virtual_device.h
//   	arch/x86/source/arch_bd_ata_driver.cpp
//   	arch/x86/source/arch_bd_ide_driver.cpp
//   	arch/x86/source/arch_bd_manager.cpp
//  	arch/x86/source/arch_bd_virtual_device.cpp
//
//  Revision 1.13  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
//  Revision 1.12  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//  Revision 1.11  2005/07/27 13:43:47  btittelbach
//  Interrupt On/Off Autodetection in Kprintf
//
//  Revision 1.10  2005/05/31 17:29:16  nomenquis
//  userspace
//
//  Revision 1.9  2005/04/25 23:09:18  nomenquis
//  fubar 2
//
//  Revision 1.8  2005/04/25 22:41:58  nomenquis
//  foobar
//
//  Revision 1.7  2005/04/25 22:40:19  btittelbach
//  Anti Warnings v0.1
//
//  Revision 1.6  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.5  2005/04/24 20:39:31  nomenquis
//  cleanups
//
//  Revision 1.4  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//  Revision 1.3  2005/04/24 10:06:08  nomenquis
//  commit to compile on different machine
//
//  Revision 1.2  2005/04/23 20:32:30  nomenquis
//  timer interrupt works
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
//----------------------------------------------------------------------

#include "ArchInterrupts.h"
#include "8259.h"
#include "structures.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "SegmentUtils.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;

// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{
  uint16 i; // disableInterrupts();
  initialise8259s();
  SegmentUtils::initialise();
  InterruptUtils::initialise();
  for (i=0;i<16;++i)
     disableIRQ(i);
}

void ArchInterrupts::enableTimer()
{
  enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
  disableIRQ(0);
}

void ArchInterrupts::enableKBD()
{
  enableIRQ(1);
  enableIRQ(9);
}

void ArchInterrupts::enableBDS()
{
  enableIRQ(2);
  enableIRQ(9);
  enableIRQ(11);
  enableIRQ(14);
  enableIRQ(15);
}

void ArchInterrupts::disableKBD()
{
  disableIRQ(1);
}

void ArchInterrupts::EndOfInterrupt(uint16 number) 
{
  sendEOI(number);
}

void ArchInterrupts::enableInterrupts()
{
     __asm__ __volatile__("sti"
   :
   :
   );
}

bool ArchInterrupts::disableInterrupts()
{
   uint32 ret_val;

 __asm__ __volatile__("pushfl\n"
                      "popl %0\n"
                      "cli"
 : "=a"(ret_val)
 :);
 
return (ret_val & (1 << 9));  //testing IF Flag

}

//tests if the InteruptFlag in EFLAGS is set
bool ArchInterrupts::testIFSet()
{
  uint32 ret_val;
  
  __asm__ __volatile__(
  "pushfl\n"
  "popl %0\n"
  : "=a"(ret_val)
  :);
  
  return (ret_val & (1 << 9));  //testing IF Flag
}
