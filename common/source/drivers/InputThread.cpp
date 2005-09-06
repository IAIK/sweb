//----------------------------------------------------------------------
//   $Id: InputThread.cpp,v 1.2 2005/09/06 09:56:50 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: InputThread.cpp,v $
//  Revision 1.1  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//----------------------------------------------------------------------

#include "InputThread.h"
#include "kernel/Scheduler.h"
#include "ArchInterrupts.h"

InputThread *InputThread::instance_=0;

void InputThread::startInputThread()
{
  if (instance_ != 0)
    return;
  
  while (kbdBufferFull())
    kbdGetScancode();
  kprintfd("Resetting Keyboard\n");
  kbdReset();
  while (kbdBufferFull())
    kbdGetScancode();
  instance_=new InputThread();
  Scheduler::instance()->addNewThread(instance_);
  ArchInterrupts::enableKBD();
}

InputThread::InputThread()
{
  name_="InputThread";
  scancode_input_ = new FiFo<uint8>(64);
  status_lights_=0;
}

InputThread::~InputThread()
{
  instance_=0;
  delete scancode_input_;
}

void InputThread::Run()
{
  uint8 scancode=0;
  while (true)
  {
    while (kbdBufferFull())
    {        
      scancode = kbdGetScancode();
      kprintfd("Emptying Keyboard Buffer content: %x\n",scancode);
      scancode_input_->put(scancode);
    }
    Scheduler::instance()->sleep();
  }
}

void InputThread::setNumlock(bool on)
{
  if (on)
    status_lights_ |= ( LIGHT_NUM & LIGHT_ALL);
  else
    status_lights_ |= ( (!LIGHT_NUM) & LIGHT_ALL);
  updateKbdLights(status_lights_);
}

void InputThread::setCapslock(bool on)
{
  if (on)
    status_lights_ |=( LIGHT_CAPS & LIGHT_ALL);
  else
    status_lights_ |=( (!LIGHT_CAPS) & LIGHT_ALL);
  updateKbdLights(status_lights_);
}

void InputThread::setScrolllock(bool on)
{
  if (on)
    status_lights_ |=( LIGHT_SCROLL & LIGHT_ALL);
  else
    status_lights_ |=( (!LIGHT_SCROLL) & LIGHT_ALL);
  updateKbdLights(status_lights_);
}
