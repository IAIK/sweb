//----------------------------------------------------------------------
//  $Id: InterruptUtils.h,v 1.1 2005/07/31 18:19:27 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: InterruptUtils.h,v $
//
//----------------------------------------------------------------------

#ifndef _INTERRUPT_UTILS_H_
#define _INTERRUPT_UTILS_H_

#include "types.h"

static uint32 const NUM_INTERRUPT_HANDLERS = 256;

typedef struct {
    uint32  number;       /*< handler number              */
    void (*handler)(); /*< pointer to handler function */
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct {
    uint16 limit;
    uint32 base;
} __attribute__((__packed__)) IDTR ;


class InterruptUtils
{
public:
  
  static void initialise();
 
  static void enableInterrupts();
  static void disableInterrupts();

  static void lidt(IDTR *idtr);

private:
  

  static InterruptHandlers handlers[NUM_INTERRUPT_HANDLERS];
};





#endif
