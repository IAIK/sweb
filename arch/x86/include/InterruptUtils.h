//----------------------------------------------------------------------
//  $Id: InterruptUtils.h,v 1.2 2005/04/27 09:19:19 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: InterruptUtils.h,v $
//  Revision 1.1  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
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
