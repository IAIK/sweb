//----------------------------------------------------------------------
//   $Id: atkbd.cpp,v 1.1 2005/09/05 23:01:24 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: atkbd.cpp,v $
//----------------------------------------------------------------------


#include "atkbd.h"

//something to get ?
bool kbdBufferFull()
{
  return (inportb(ATKBD_CTRL) & 1);
}

//get scancode
uint8 kbdGetScancode()
{
  return inportb(ATKBD_DATA);
}

//reset
void kbdReset()
{
  outportb(ATKBD_DATA,0xF6);
  inportb(ATKBD_DATA); // should be ACK
}


void updateKbdLights(uint8 status)
{
  outportb(ATKBD_DATA,0xED);
  inportb(ATKBD_DATA); // should be ACK
  outportb(ATKBD_DATA,status);
  inportb(ATKBD_DATA); // should be ACK
}
