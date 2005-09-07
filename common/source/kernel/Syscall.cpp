//----------------------------------------------------------------------
//   $Id: Syscall.cpp,v 1.5 2005/09/07 00:33:52 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Syscall.cpp,v $
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

uint32 Syscall::syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5)
{
  uint32 return_value=0;
  
  kprintfd("Syscall %d called with arguments %d %d %d %d %d\n",syscall_number, arg1, arg2, arg3, arg4, arg5);
  
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
  kprintfd("Syscall EXIT called, exit_code: %d\n",exit_code);
  currentThread->kill();
}

uint32 Syscall::write(uint32 fd, pointer buffer, uint32 size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  assert(buffer < 2U*1024U*1024U*1024U);
  if (fd == fd_stdout) //stdout
  {
    char *kernel_buffer = new char[size+1];
    kernel_buffer[size]=0;
    //don't really need to copy, but do it anyway for security reasons
    ArchCommon::memcpy((pointer) kernel_buffer, buffer, size);
    kprintfd("Syscall::write: %s\n",kernel_buffer);
    kprintf("\n%s\n",kernel_buffer);
    delete kernel_buffer;
  }
  return size;
}

uint32 Syscall::read(uint32 fd, pointer buffer, uint32 count)
{
  uint32 num_read = count;
  if (fd == fd_stdin) //stdin
  {
    //Achtung, wir können hier nicht blocken und müssen einen Threadswitch vermeiden
    //was aber wenn wir blocken wollen ???

    //Input Beispiel: Direkt Scancodes für den Userspace und dort decoden
    //~ uint32 count_ahead = InputThread::getInstance()->countAhead();
    //~ if (count_ahead < num_read)
      //~ num_read = count_ahead;
    
    //~ uint8 mybuffer[num_read];
    
    //~ kprintfd("Syscall::read: %d to read\n",num_read);
    //~ for (uint32 c=0; c<num_read; ++c)
    //~ {
      //~ mybuffer[c] = InputThread::getInstance()->getScancode();
      //~ kprintfd("Syscall::read: got %x\n",((char*)buffer)[c]);
    //~ }
    
    //~ ArchCommon::memcpy(buffer,(pointer) &mybuffer,num_read);
    
  }
  return num_read;
}

uint32 Syscall::clone() 
{
  uint32 child_pid = 999; //no pids yet
  
  //clone process
 
  return child_pid; //we are parent
}
