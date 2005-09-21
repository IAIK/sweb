//----------------------------------------------------------------------
//  $Id: ArchInterrupts.h,v 1.13 2005/09/21 12:08:10 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.h,v $
//  Revision 1.12  2005/09/20 20:19:48  btittelbach
//  doxyfication
//
//  Revision 1.11  2005/09/20 20:11:18  btittelbach
//  doxification
//
//  Revision 1.10  2005/09/20 17:43:55  lythien
//  *** empty log message ***
//
//  Revision 1.9  2005/09/18 20:25:05  nelles
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
//  Revision 1.8  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
//  Revision 1.7  2005/09/05 23:01:23  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//  Revision 1.6  2005/07/27 13:43:47  btittelbach
//  Interrupt On/Off Autodetection in Kprintf
//
//  Revision 1.5  2005/04/25 22:40:18  btittelbach
//  Anti Warnings v0.1
//
//  Revision 1.4  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.3  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//  Revision 1.2  2005/04/23 20:32:30  nomenquis
//  timer interrupt works
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
//----------------------------------------------------------------------


#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#include "types.h"

/** @class ArchInterrupts
 *
 * Collection of architecture dependant functions conerning Interrupts
 *
 */
class ArchInterrupts
{
public:

/**
 * the timer handler callback will return 0 if nothing happened
 * or 1 if it wants to have a task switch
 * details of the task switch have to be set through other methods
 */
  typedef uint32 (*TimerHandlerCallback)();

/** @initialise
 *
 * gives the arch a chance to set things up the way it wants to
 *
 */
  static void initialise();

/** @enableTimer
 *
 * enables the Timer IRQ (0)
 *
 */
  static void enableTimer();

/** @disableTimer
 *
 * disables the Timer IRQ (0)
 *
 */
  static void disableTimer();

/** @enableKBD
 *
 * enables the Keyboard IRQ (1)
 *
 */
  static void enableKBD();

/** @enableBDS
 *
 * enables the BlockDevice IRQs (13,14,15)
 *
 */
  static void enableBDS();

/** @disableKBD
 *
 *  disables the Keyboard IRQ (1)
 *
 */
  static void disableKBD();

/** @EndOfInterrupt
 *
 * Signals EOI to the Interrupt-Controller, so the Controller
 * can resume sending us Interrupts
 *
 * @param number of Interrupt that finished, so we know which Interrupt-Controller to signal
 *
 */
  static void EndOfInterrupt(uint16 number);


/** @enableInterrupts
 *
 * enable interrupts, no more lazy linear code execution time ;-)
 * (using sti on x86)
 */
  static void enableInterrupts();

/** @disableInterrupts
 *
 * disables Interrupts (by using cli (clear interrupts) on x86)
 * @return bool true if Interrupts were enabled, false otherwise
 */
  static bool disableInterrupts();

/** @testIFSet
 *
 * on x86: tests if the IF Flag in EFLAGS is set, aka if the Interrupts are enabled
 * 
 * @return bool true if Interrupts are enabled, false otherwise
 */
  static bool testIFSet();
  //static void setOldInterruptState(uint32 const &flags);
};









#endif
