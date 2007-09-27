//----------------------------------------------------------------------
//   $Id: atkbd.cpp,v 1.1 2005/09/21 03:33:52 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: atkbd.cpp,v $
//  Revision 1.2  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//  Revision 1.1  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
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

bool kbd_light_numlock = 0;
bool kbd_light_capslock = 0;
bool kbd_light_scrolllock = 0;

void kbdSetNumlock(bool on)
{
  kbd_light_numlock=on;
  updateKbdLights((LIGHT_NUM & kbd_light_numlock)
               | (LIGHT_CAPS & kbd_light_capslock) 
               | (kbd_light_numlock & kbd_light_scrolllock));
}
void kbdSetCapslock(bool on)
{
  kbd_light_capslock=on;
  updateKbdLights((LIGHT_NUM & kbd_light_numlock)
               | (LIGHT_CAPS & kbd_light_capslock) 
               | (kbd_light_numlock & kbd_light_scrolllock));

}
void kbdSetScrolllock(bool on)
{
  kbd_light_scrolllock=on;
  updateKbdLights((LIGHT_NUM & kbd_light_numlock)
               | (LIGHT_CAPS & kbd_light_capslock) 
               | (kbd_light_numlock & kbd_light_scrolllock));

}
