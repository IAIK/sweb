//----------------------------------------------------------------------
//   $Id: Syscall.cpp,v 1.10 2005/09/21 23:03:35 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Syscall.cpp,v $
//  Revision 1.9  2005/09/21 21:29:45  btittelbach
//  make kernel readline do less, as its suppossed to
//
//  Revision 1.8  2005/09/16 15:47:41  btittelbach
//  +even more KeyboardInput Bugfixes
//  +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//  +Thread now remembers its Terminal
//  +Syscalls are USEABLE !! :-) IF=1 !!
//  +Syscalls can block now ! ;-) Waiting for Input...
//  +more other Bugfixes
//
//  Revision 1.7  2005/09/16 00:54:13  btittelbach
//  Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
//  Revision 1.6  2005/09/15 18:47:07  btittelbach
//  FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//  IdleThread now uses hlt instead of yield.
//
//  Revision 1.5  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//  Revision 1.4  2005/09/06 09:56:50  btittelbach
//  +Thread Names
//  +stdin Test Example
//
//  Revision 1.3  2005/09/03 21:54:45  btittelbach
//  Syscall Testprogramm, actually works now ;-) ;-)
//  Test get autocompiled and autoincluded into kernel
//  one kprintfd bug fixed
//
//  Revision 1.2  2005/08/26 13:58:24  nomenquis
//  finally even the syscall handler does that it is supposed to do
//
//  Revision 1.1  2005/08/03 09:54:43  btittelbach
//  Syscall Files, unfinished as of yet
//

#include "Syscall.h"
#include "syscall-definitions.h"
#include "assert.h"
#include "../console/kprintf.h"
#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "console/Terminal.h"

uint32 Syscall::syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5)
{
  uint32 return_value=0;
  
  kprintfd("Syscall %d called with arguments %d(=%x) %d(=%x) %d(=%x) %d(=%x) %d(=%x)\n",syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  
  switch (syscall_number)
  {
    case sc_exit:
      exit(arg1);
      break;
    case sc_clone:
      return_value = clone();
      break;
    case sc_write:
      return_value =  write(arg1,arg2,arg3);
      break;
    case sc_read:
      return_value =  read(arg1,arg2,arg3);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %d\n",syscall_number);
  }  
  return return_value;
}

void Syscall::exit(uint32 exit_code) 
{
  kprintfd("Syscall::EXIT: called, exit_code: %d\n",exit_code);
  currentThread->kill();
}

uint32 Syscall::write(uint32 fd, pointer buffer, uint32 size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  assert(buffer < 2U*1024U*1024U*1024U);
  if (fd == fd_stdout) //stdout
  {
    kprintfd("Syscall::write: %B\n",(char*) buffer,size);
    kprint_buffer((char*)buffer,size);
  }
  return size;
}

uint32 Syscall::read(uint32 fd, pointer buffer, uint32 count)
{
  assert(buffer < 2U*1024U*1024U*1024U);
  uint32 num_read = 0;
  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    kprintfd("Syscall::read: %B\n",(char*) buffer,num_read);
    for (uint32 c=0; c<num_read; ++c)
      kprintfd("%c(%x) ",((char*)buffer)[c],((char*)buffer)[c]);
    kprintfd("\n");

  }
  return num_read;
}

uint32 Syscall::clone() 
{
  uint32 child_pid = 999; //no pids yet
  
  //clone process
 
  return child_pid; //we are parent
}
