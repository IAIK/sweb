/**
 * $Id: panic.cpp,v 1.1 2005/08/01 08:28:34 nightcreature Exp $
 *
 * $Log: panic.c,v $
 *
 */

#include <types.h>
#include <Thread.h>
#include <Scheduler.h>
#include <ArchInterrupts.h>
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"

// pointers to the start and end of the kernel code
extern uint8* LS_Code;
extern uint8* LS_Data;

// pointers to the symbol table and symbol strings
extern uint8* stab_start_address_nr;
extern uint8* stab_end_address_nr;

extern uint8* stabstr_start_address_nr;
extern uint8* stabstr_end_address_nr;

pointer KERNEL_CODE_START  = (pointer)&LS_Code;
pointer KERNEL_CODE_END    = (pointer)&LS_Data;

pointer STAB_START         = (pointer)&stab_start_address_nr;
pointer STAB_END           = (pointer)&stab_end_address_nr;

pointer STABSTR_START      = (pointer)&stabstr_start_address_nr;
pointer STABSTR_END        = (pointer)&stabstr_end_address_nr;

uint32 MAX_FN_DEPTH         = 256;

  /// \brief kernel panict function writes the message and the stack trace to the screen and dies after that

  void kpanict ( uint8 * message ) 
  {
//     ArchInterrupts::disableInterrupts();
    
//     uint32* stack = (uint32*) currentThread->getStackStartPointer();
//     stabs_out * symTablePtr = (stabs_out *) STAB_START;
    
//     kprintf( "KPANICT: stack is > %x ",  stack );

//     uint32 * esp_reg = 0;
    
//     __asm__ __volatile__(" \
//      pushl %%eax\n \
//      movl %%esp, %%eax\n \
//      movl %%eax, %0\n \
//      popl %%eax\n" 
//       : "=g" (esp_reg) 
//      );

//     kprintf( "esp_reg is > %x\n",  esp_reg );
     
//     for( uint32 * i = (esp_reg); i < stack; i++ )
//     {
//       if( (*i >= KERNEL_CODE_START && *i <= KERNEL_CODE_END) )
//       //|| ( *((uint32 *)*i) >= KERNEL_CODE_START && *((uint32 *)*i) <= KERNEL_CODE_END ) )
//       {
//         kprintf( "i: %x, ",  i );
//         kprintf( "*i: %x ", *i );
//         kprintf( "**i: %x \n", *((uint32 *)*i) );
        
//         //|| symTablePtr->n_value == *((uint32 *)g)
        
//         uint32 g = *i;
//         for( g = *i; g > (*i - MAX_FN_DEPTH); g-- )
//         {
//           for( symTablePtr = (stabs_out *) STAB_START ; symTablePtr < (stabs_out *) STAB_END; symTablePtr++ )
//           {
//             if( symTablePtr->n_value == g  )
//             {
//               if( symTablePtr->n_value < KERNEL_CODE_START 
//               || symTablePtr->n_value > KERNEL_CODE_END )
//               {
//                 kprintf( "!" );
//                 break;
//               }
                
//               if( symTablePtr->n_type == 0x24 )
//               {
//                 kprintf("v: %x, t %x ", symTablePtr->n_value, symTablePtr->n_type );
              
//                 kprintf(" %s \n", ( STABSTR_START + symTablePtr->n_strx ) );
                
//                 g = (*i - MAX_FN_DEPTH) - 1; // sub 1 to be sure
//               }
//               else
//                 kprintf( "!" );
//             } // if         
//           } // for stabs
//         } // for g
        
//       }
//     }
    
//     kprintf("%s \n", message );   
    
//     ArchInterrupts::disableInterrupts();
//     ArchInterrupts::disableTimer();
//     //disable other IRQ's ???
    
//     for(;;) // taking a break from the cruel world
//     {
//       __asm__ __volatile__(" \
//        hlt\n"
//       );
//     }
    
//     kprintf("MAJOR KERNEL PANIC!: Should never reach here\n");
    
//     pointer i = 0;
              
//     for( ; symTablePtr < (stabs_out *) STAB_END; symTablePtr++ )
//     { 
//         if( symTablePtr->n_type == 0x24 )
//         {
//           if( (i++ % 10) == 0 )      
//             kprintf("index  | type | oth. | desc | address   |  function\n");
        
//           kprintf( "%x    %x  %x   %x     %x  %s    \n", 
//           symTablePtr->n_strx,
//           symTablePtr->n_type, 
//           symTablePtr->n_other,
//           symTablePtr->n_desc,
//           symTablePtr->n_value,
//           ( STABSTR_START + symTablePtr->n_strx ) );
          
//         }
//     }
        
  }
