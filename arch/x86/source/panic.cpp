#include <types.h>
#include <Thread.h>
#include <Scheduler.h>
#include "kprintf.h"

extern void* kernel_start_address;
extern void* kernel_end_address;

extern void* stab_start_address;
extern void* stab_end_address;

pointer KERNEL_CODE_START  = (pointer)&kernel_start_address;
pointer KERNEL_CODE_END    = (pointer)&kernel_end_address;

pointer STAB_START         = (pointer)&stab_start_address;
pointer STAB_END           = (pointer)&stab_end_address;

  /// \brief kernel panict function writes the message and the stack trace to the screen and dies after that
  /// \param message A null terminated string
  void kpanict ( uint8 * message ) 
  {
    pointer stack = currentThread->getStackStartPointer();
    
    for( pointer i = stack; i > 0; i--) 
    {
      kprintf("*I is > %x, KST is > %x, KEND is > %x \n", *( (uint32 *) i), KERNEL_CODE_START, KERNEL_CODE_END );
      // check whether the address is in code section of kernel
      if( i >= KERNEL_CODE_START && i <= KERNEL_CODE_END )
      {
        kprintf("Called from : %x", *( (uint32 *) i) );// print it out
      }
      
      // find a corresponding symbol in symbols
    }
    
    kprintf( "%s", message ); 
  }
