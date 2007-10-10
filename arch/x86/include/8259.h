/**
 * @file 8259.h
 *
 */
 
#ifndef _8259_H_
#define _8259_H_

#include "types.h"
#include "ports.h"
#define PIC_1_CONTROL_PORT 0x20
#define PIC_2_CONTROL_PORT 0xA0
#define PIC_1_DATA_PORT 0x21
#define PIC_2_DATA_PORT 0xA1

/**
 * sends the initialisation and operational command words to CPU
 *
 */
void initialise8259s();

extern uint32 cached_mask;

/**
 * enables the interrupt Request with the given number 0 to 15
 *
 */
void enableIRQ(uint16 number);

/**
 * disables the interrupt Request with the given number 0 to 15
 *
 */
void disableIRQ(uint16 number);

/**
 * sends the EOI signal to a Programmable Interrupt Controller (PIC)
 * to indicate the completion of interrupt processing for the given
 * interrupt.
 *
 */
void sendEOI(uint16 number);

#endif
