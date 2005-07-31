//----------------------------------------------------------------------
//  $Id: 8259.h,v 1.1 2005/07/31 17:49:03 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: 8259.h,v $
//
//----------------------------------------------------------------------


#ifndef _8259_H_
#define _8259_H_

#include "types.h"
#include "ports.h"
#define PIC_1_CONTROL_PORT 0x20
#define PIC_2_CONTROL_PORT 0xA0
#define PIC_1_DATA_PORT 0x21
#define PIC_2_DATA_PORT 0xA1


void initialise8259s();

extern uint32 cached_mask;

void enableIRQ(uint16 number);
void disableIRQ(uint16 number);

#endif
