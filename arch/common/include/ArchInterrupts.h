//----------------------------------------------------------------------
//  $Id: ArchInterrupts.h,v 1.11 2005/09/20 20:11:18 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.h,v $
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
 * enables the Timer
 *
 */
  static void enableTimer();

/** @disableTimer
 *
 * disables the Timer
 *
 */
  static void disableTimer();

/** @enableKBD
 *
 * enables the KBD
 *
 */
  static void enableKBD();

/** @enableBDS
 *
 * enables the BDS
 *
 */
  static void enableBDS();

/** @disableKBD
 *
 *  disables the KBD
 *
 */
  static void disableKBD();

/** @EndOfInterrupt
 *
 * @param number
 *
 */
  static void EndOfInterrupt(uint16 number);


/** @enableInterrupts
 *
 * enable interrupts, no matter what, this is bad
 *
 */
  static void enableInterrupts();

/** @disableInterrupts
 *
 * disable interrupts
 *
 */
  static bool disableInterrupts();

/** @testIFSet
 *
 * 
 *
 */
  static bool testIFSet();
  //static void setOldInterruptState(uint32 const &flags);
};









#endif
