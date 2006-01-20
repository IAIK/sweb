//----------------------------------------------------------------------
//  $Id: head.S,v 1.3 2006/01/20 07:20:04 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: head.S,v $
//  Revision 1.2  2005/08/11 16:55:47  nightcreature
//  preview commit only for robert ;-)
//
//  Revision 1.1  2005/08/01 08:27:05  nightcreature
//  assembler files need for xen (boot up and "interupt" entry)
//
//
//----------------------------------------------------------------------


 
#include <os.h>

.equ HAHA_JETZT, 4567
 
.section __xen_guest
	.ascii	"GUEST_OS=linux,GUEST_VER=2.6"
  .ascii  ",XEN_VER=xen-3.0"
  .ascii  ",VIRT_BASE=0x80000000"
  .ascii  ",PAE=no"
	.ascii	",LOADER=generic"
//  .ascii ",SHADOW=translate" //okay for xen 3.0
//	.ascii	",PT_MODE_WRITABLE" //from xen 2.0, don't know if okay for xe n 3.0
	.byte	0
 
.text

.globl _start, shared_info, console_page_dummy
                        
_start:
        cld
        //lss stack_start,%esp
        push %esp
        push %esi
        call start_kernel

stack_start:
	.long stack+8192, __KERNEL_DS

        /* Unpleasant -- the PTE that maps this page is actually overwritten */
        /* to map the real shared-info page! :-)                             */
        .org 0x1000
shared_info:
        .org 0x2000	
console_page_dummy:
        .org 0x3000	
