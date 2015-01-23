/**
 * @file InterruptUtils.h
 *
 * Block devices update.
 * See BDRequest and BDManager on how to use this.
 * Currently ATADriver is functional. The driver tries to detect if IRQ
 * mode is available and adjusts the mode of operation. Currently PIO
 * modes with IRQ or without it are supported.
 *
 * TODO:
 * - add block PIO mode to read or write multiple sectors within one IRQ
 * - add DMA and UDMA mode :)
 *
 */


#ifndef _INTERRUPT_UTILS_H_
#define _INTERRUPT_UTILS_H_

#include "types.h"

#define PIC_IRQ_ENABLESET   0x2
#define PIC_IRQ_STATUS   0x0

//#define REG_LOAD    0x00 TODO: should this be here?
#define REG_CTRL    0x02
#define REG_INTCLR    0x03
#define REG_INTSTAT   0x04
#define REG_INTMASKED   0x05
#define REG_BGLOAD    0x06
#define REG_LOAD    0x07
#define REG_CURRENT    0x07

#define CTRL_ENABLE     0x80
#define CTRL_MODE_PERIODIC  0x40
#define CTRL_INT_ENABLE   (1<<5)
#define CTRL_DIV_NONE   0x00
#define CTRL_SIZE_32    0x02

#define ARM4_XRQ_RESET   0x00
#define ARM4_XRQ_UNDEF   0x01
#define ARM4_XRQ_SWINT   0x02
#define ARM4_XRQ_ABRTP   0x03
#define ARM4_XRQ_ABRTD   0x04
#define ARM4_XRQ_RESV1   0x05
#define ARM4_XRQ_IRQ     0x06
#define ARM4_XRQ_FIQ     0x07

extern "C" void exceptionHandler(uint32 type);

#endif
