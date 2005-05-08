#include <types.h>
#include <Thread.h>
#include <Scheduler.h>
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"

// pointers to the start and end of the kernel code
extern void* LS_Code;
extern void* LS_Data;

// pointers to the symbol table and symbol strings
extern void* stab_start_address_nr;
extern void* stab_end_address_nr;

extern void* stabstr_start_address_nr;
extern void* stabstr_end_address_nr;

pointer KERNEL_CODE_START  = (pointer)&LS_Code;
pointer KERNEL_CODE_END    = (pointer)&LS_Data;

pointer STAB_START         = (pointer)&stab_start_address_nr;
pointer STAB_END           = (pointer)&stab_end_address_nr;

pointer STABSTR_START      = (pointer)&stabstr_start_address_nr;
pointer STABSTR_END        = (pointer)&stabstr_end_address_nr;

  /// \brief kernel panict function writes the message and the stack trace to the screen and dies after that
  /// \param message A null terminated string
  void kpanict ( uint8 * message ) 
  {
    pointer stack = currentThread->getStackStartPointer();
    
    kprintf( "stack is > %x, KST is > %x, KEND is > %x \n",  stack, KERNEL_CODE_START, KERNEL_CODE_END );
    
    stabs_out * symTablePtr = (stabs_out *) STAB_START;

    kprintf( "SST is > %x, SEND is > %x \n", STAB_START, STAB_END );
    kprintf( "SstrST is > %x, SstrEND is > %x \n", STABSTR_START, STABSTR_END );
      
    pointer i = 0; 
              
    for( ; symTablePtr < (stabs_out *) STAB_END; symTablePtr++ )
    { 
        if( symTablePtr->n_type == 0x24 )
        {
          if( (i++ % 10) == 0 )      
            kprintf("index  | type | oth. | desc | address   |  function\n");
        
          kprintf( "%x    %x  %x   %x     %x  %s    \n", 
          symTablePtr->n_strx,
          symTablePtr->n_type, 
          symTablePtr->n_other,
          symTablePtr->n_desc,
          symTablePtr->n_value,
          ( STABSTR_START + symTablePtr->n_strx ) );
          
        }
    }
    
//    page fault to stop everything
//    kprintf("str > %s \n", 0x80014020 + 0x80050000 );  
           
/*    for( pointer i = stack; i > 0; i--) 
    {
      kprintf("I is > %x \n ", i ); 
      kprintf("*I is > %x,  \n", *( (uint32 *) i) );
      // check whether the address is in code section of kernel
      if( i >= KERNEL_CODE_START && i <= KERNEL_CODE_END )
      {
        kprintf("Called from : %x", *( (uint32 *) i) );// print it out
      }
      
      // find a corresponding symbol in symbols
    }*/
    
    kprintf( "%s", message ); 
  }
