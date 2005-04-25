;
; $Id: boot.s,v 1.13 2005/04/25 23:09:18 nomenquis Exp $
;
; $Log: boot.s,v $
; Revision 1.12  2005/04/24 16:58:04  nomenquis
; ultra hack threading
;
; Revision 1.11  2005/04/23 11:56:34  nomenquis
; added interface for memory maps, it works now
;
; Revision 1.10  2005/04/22 20:14:25  nomenquis
; fix for crappy old gcc versions
;
; Revision 1.9  2005/04/22 17:40:57  nomenquis
; cleanup
;
; Revision 1.8  2005/04/22 17:21:39  nomenquis
; added TONS of stuff, changed ZILLIONS of things
;
; Revision 1.7  2005/04/21 21:31:24  nomenquis
; added lfb support, also we now use a different grub version
; we also now read in the grub multiboot version
;
; Revision 1.6  2005/04/20 20:42:56  nomenquis
; refined debuggability for bootstrapping
; also using 4m mapping for 3g ident and 4k mapping with 4 ptes (but only one used) for 2g
;
; Revision 1.5  2005/04/20 15:26:35  nomenquis
; more and more stuff actually works
;
; Revision 1.3  2005/04/20 08:06:17  nomenquis
; the overloard (thats me) managed to get paging with 4m pages to work.
; kernel is now at 2g +1 and writes something to the fb
; w00t!
;
; Revision 1.2  2005/04/20 07:09:59  nomenquis
; added inital paging for the kernel, plus mapping to two gigs
; hoever, the kernel now is at 1meg phys and 2gig + 1 meg virtual due to these 4m pages
;
; Revision 1.1  2005/04/12 17:46:43  nomenquis
; added lots of files
;
;
;

LINK_BASE            EQU     80100000h              ; Base address (virtual)
LOAD_BASE           EQU     00100000h              ; Base address (physikal)
BASE                EQU     (LINK_BASE - LOAD_BASE) ; difference to calculate (virtual)
PHYS_OFFSET EQU 0C0000000h

; this is a magic number which will be at the start
; of the data segment
; used to verify that the bootloader really loaded everything
DATA_SEGMENT_MAGIC equ 3544DA2Ah

EXTERN text_start_address, text_end_address,bss_start_address, bss_end_address, kernel_end_address

; this is really really bad voodoo ...
; grub needs this, or it will refuse to boot
MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_WANT_VESA equ 1<<2
MULTIBOOT_AOUT_KLUDGE   equ 1<<16
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE|MULTIBOOT_WANT_VESA
;MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

%macro writeTestOnScreen 0
   ;mov word[0B8000h], 9F54h
   ;mov word[0B8002h], 9F65h
   ;mov word[0B8004h], 9F73h
   ;mov word[0B8006h], 9F74h
   ;mov word[0B800Ah], 9F21h
   mov word[0C00B8020h], 9F54h
   mov word[0C00B8022h], 9F65h
   mov word[0C00B8024h], 9F73h
   mov word[0C00B8026h], 9F74h
   mov word[0C00B802Ah], 9F21h
%endmacro
%macro writeTestOnScreenUnMapped 0
   mov word[0B8000h], 9F54h
   mov word[0B8002h], 9F65h
   mov word[0B8004h], 9F73h
   mov word[0B8006h], 9F74h
   mov word[0B800Ah], 9F21h

%endmacro

%macro halt 0
   jmp short $ ; now what does this do? it just jumps to this instructing untill the end of all times
%endmacro

; Text section == Code that can be exectuted
SECTION .text
BITS 32 ; we want 32bit code

; this is where well start
; first check if the loader did a good job
GLOBAL entry
entry:

   ; we get these from grub
   mov [multi_boot_magic_number-BASE], eax;
   mov [multi_boot_structure_pointer-BASE], ebx;


   mov edi,0B8000h; load bss address
   mov ecx,0B8FA0h; end of bss and stack (!), this symbol is at the very end of the kernel
   sub ecx, edi ; how much data do we have to clear
   xor eax, eax ; we want to fill with 0
   rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0



    mov word[0B8000h], 9F30h


   mov eax,[ds_magic - BASE] ; value of memory pointed to by ds_magic symbol into eax
   cmp eax, DATA_SEGMENT_MAGIC

   ; comment this je to just see a few characters on the screen, really impressive :)
   je data_segment_ok ; if its the same then everything is good

; if we end up here the data segment is _not_ ok
; write something to the vga memory and halt

   mov word[0B8000h], 9F44h
   mov word[0B8002h], 9F61h
   mov word[0B8004h], 9F74h
   mov word[0B8006h], 9F61h
   mov word[0B800Ah], 9F45h
   mov word[0B800Ch], 9F72h
   mov word[0B800Eh], 9F72h
   mov word[0B8010h], 9F6fh
   mov word[0B8012h], 9F72h

   jmp short $ ; now what does this do? it just jumps to this instructing untill the end of all times

data_segment_ok:

    mov word[0B8002h], 9F31h

   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space
   lgdt [gdt_ptr - BASE]

   ; now prepare all the segment registers to use our segments
   mov ax, LINEAR_DATA_SEL
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov gs,ax

   ; use these segments
   jmp LINEAR_CODE_SEL:(now_using_segments - BASE)

now_using_segments:


   ; next thing to do to be c compliant is to
   ; clear the bss (uninitialised data)
   ; we also clear the stack
   
   
   mov edi,bss_start_address - BASE; load bss address
   mov ecx, bss_end_address - BASE; end of bss and stack (!), this symbol is at the very end of the kernel
   sub ecx, edi ; how much data do we have to clear
   xor eax, eax ; we want to fill with 0
   rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0




   ; setup the stack pointer to point to our stack in the just cleared bss section

    mov esp,stack - BASE

    mov word[0B8004h], 9F32h

EXTERN parseMultibootHeader;

mov eax,parseMultibootHeader - BASE
call eax

EXTERN initialiseBootTimePaging

mov eax,initialiseBootTimePaging - BASE
call eax

    mov word[0B8006h], 9F33h


mov     eax,kernel_page_directory_start - BASE; eax = &PD
mov     cr3,eax         ; cr3 = &PD


; set bit 0x4 to 1 in cr4 to enable PSE
; need a pentium cpu for this but it will give us 4mbyte pages
mov eax,cr4;
or eax, 0x00000010;
mov cr4,eax;

;  2) setting CR0's PG bit.

   mov word[0B8008h], 9F34h


mov     eax,cr0
or      eax,0x80010001   ; Set PG bit
mov     cr0,eax         ; Paging is on!

jmp     $ + 2          ; Flush the instruction queue.


   mov word[0B800Ah], 9F35h

 mov edi,stack_start; load bss address
 mov ecx, stack; end of bss and stack (!), this symbol is at the very end of the kernel
 sub ecx, edi ; how much data do we have to clear
 xor eax, eax ; we want to fill with 0
 rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

 mov esp,stack



   mov word[0B800Ch], 9F36h

mov eax, PagingMode
call eax


PagingMode:

   mov word[0C00B800Eh], 9F38h



   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space
   lgdt [gdt_ptr_new]

   ; now prepare all the segment registers to use our segments
   mov ax, LINEAR_DATA_SEL
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov gs,ax

   ; use these segments
   jmp LINEAR_CODE_SEL:(now_using_segments_new)

now_using_segments_new:

   mov word[0C00B8010h], 9F39h



EXTERN removeBootTimeIdentMapping

call removeBootTimeIdentMapping


   mov word[0C00B8012h], 4330h

mov     eax,kernel_page_directory_start - BASE; eax = &PD
mov     cr3,eax         ; cr3 = &PD

   mov word[0C00B8014h], 4331h

;EXTERN initInterruptHandlers
;mov eax,initInterruptHandlers;
;call eax;

 ;  mov word[0C00B8016h], 4332h

   ; set up interrupt handlers, then load IDT register
;   mov ecx,(idt_end - idt) >> 3 ; number of exception handlers
;   mov edi,idt
;   mov esi,isr0
;do_idt:
;   mov eax,esi			; EAX=offset of entry point
;   mov [edi],ax			; set low 16 bits of gate offset
;   shr eax,16
;   mov [edi + 6],ax		; set high 16 bits of gate offset
;   add edi,8			; 8 bytes/interrupt gate
;   add esi,(isr1 - isr0)		; bytes/stub
;   loop do_idt

;   mov eax, idt_ptr

;   mov word[0C00B8018h], 4334h

;   lidt [eax]

   mov word[0C00B801Ah], 4335h

;call inivalidate_ident_mapping
;call inivalidate_ident_mapping


EXTERN panic

;push  0
;call panic



; GRUB 0.90 leaves the NT bit set in EFLAGS. The first IRET we attempt
; will cause a TSS-based task-switch, which will cause Exception 10.
; Let's prevent that:



   push dword 2
   popf

EXTERN startup ; tell the assembler we have a main somewhere
   mov word[0C00B801Ch], 4336h

   call startup ; hellloooo, here we are in c !
   jmp $ ; suicide

global reload_segements
reload_segements:
   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space
   lgdt [gdt_ptr_very_new]

   ; now prepare all the segment registers to use our segments
   mov ax, LINEAR_DATA_SEL
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov gs,ax

   ; use these segments
   jmp LINEAR_CODE_SEL:(reload_segments_new)

reload_segments_new:

    ret;
    
    



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;,
;; this will help us to boot, this way we can tell grub
;; what to do

SECTION .mboot
ALIGN 4
GLOBAL mboot
mboot:
   dd MULTIBOOT_HEADER_MAGIC
   dd MULTIBOOT_HEADER_FLAGS
   dd MULTIBOOT_CHECKSUM
; aout kludge. These must be PHYSICAL addresses
   dd mboot - BASE
   dd text_start_address - BASE
   dd bss_start_address  - BASE
   dd kernel_end_address - BASE
;   dd mboot - BASE
;   dd 0
;   dd 0
;   dd 0
;   dd 0
   dd 0 ; mode 
   dd 800 ;width
   dd 600 ; height
   dd 32; depth

section .text
; this is the data section
; you can see that the first thing we have here is our magic value

GLOBAL diediedie
diediedie:
jmp $

SECTION .gdt_stuff



; this descritor is _REALLY_ braindead
; have a look at http://www.csee.umbc.edu/~plusquel/310/slides/micro_arch2.html
align 4096
gdt:

; this is the first global descriptor table
; this stuff is used for segmenting, we dont need anything special
; here to just boot and present some things



; NULL descriptor
	dw 0			; limit 15:0
	dw 0			; base 15:0
	db 0			; base 23:16
	db 0			; type
	db 0			; limit 19:16, flags
	db 0			; base 31:24

; unused descriptor
	dw 0
	dw 0
	db 0
	db 0
	db 0
	db 0


; the first interesting descriptor this one is for data
   LINEAR_DATA_SEL	equ	$-gdt
	dw 0FFFFh ; the limit, since the page-granular bit is turned on this is shifted 12 bytes to the left
             ; in our case this means that the segment spawns the while 32bit address space
	dw 0
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0


; basically the same thing as before, but we also need a descriptor for code
   LINEAR_CODE_SEL	equ	$-gdt
	dw 0FFFFh
	dw 0
	db 0
	db 9Ah			; present,ring 0,code,non-conforming,readable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0
   
; the first interesting descriptor this one is for data
   LINEAR_USR_DATA_SEL	equ	$-gdt
	dw 0FFFFh ; the limit, since the page-granular bit is turned on this is shifted 12 bytes to the left
             ; in our case this means that the segment spawns the while 32bit address space
	dw 0
	db 0
	db 0F2h			; present, ring 3, data, expand-up, writable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0


; basically the same thing as before, but we also need a descriptor for user code
   LINEAR_USR_CODE_SEL	equ	$-gdt
	dw 0FFFFh
	dw 0
	db 0
	db 0FAh			; present,ring 3,code,non-conforming,readable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0
  gdt_end:

; basically the same thing as before, but we also need a descriptor for user code
global tss_selector
tss_selector:
 TSS_SEL	equ	$-gdt
	dw 0
	dw 0
	db 0
	db 0			; present,ring 3,code,non-conforming,readable
	db 0                 ; page-granular (4 gig limit), 32-bit
	db 0
gdt_real_end:

gdt_ptr:
	dw gdt_end - gdt - 1
	dd gdt - BASE
  
global gdt_ptr_new
gdt_ptr_new:

	dw gdt_end - gdt - 1
	dd gdt
	
global gdt_ptr_very_new
gdt_ptr_very_new:

	dw gdt_real_end - gdt - 1
	dd gdt
	

%rep 1024
 dd 0
%endrep

; 256 ring 0 interrupt gates
; we certainly miss one ring 3 interrupt gate (for syscalls)
SECTION .idt_stuff
GLOBAL idt
idt:
%rep 256
	dw 0				; offset 15:0
	dw LINEAR_CODE_SEL		; selector
	db 0				; (always 0 for interrupt gates)
	db 8Eh				; present,ring 0,'386 interrupt gate
	dw 0				; offset 31:16
%endrep
idt_end:

GLOBAL idt_ptr

idt_ptr:
   dw idt_end - idt - 1    ; IDT limit
   dd idt                  ; linear adr of IDT

ds_magic:
   dd DATA_SEGMENT_MAGIC

SECTION .data
GLOBAL multi_boot_magic_number
multi_boot_magic_number:
	dd 0
GLOBAL multi_boot_structure_pointer
multi_boot_structure_pointer:
	dd 0
   
SECTION .bss
   ; here we create lots of room for our stack (actually this is one by the resd 1024)
   GLOBAL stack_start
stack_start:
   resd 65536
   GLOBAL stack
stack:
SECTION .paging_stuff
GLOBAL kernel_page_directory_start
align 4096
kernel_page_directory_start:
  resd 4096
GLOBAL kernel_page_tables_start:
align 4096
kernel_page_tables_start:
  rest 4096
  rest 4096
  rest 4096
  rest 4096
