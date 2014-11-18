
LINK_BASE           EQU     80000000h              ; Base address (virtual) (kernel is linked to start at this address)
LOAD_BASE           EQU     00100000h              ; Base address (physikal) (kernel is actually loaded at this address)
BASE                EQU     (LINK_BASE - LOAD_BASE) ; difference to calculate physical adress from virtual address until paging is set up
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
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; Text section == Code that can be exectuted

SECTION .text
BITS 32 ; we want 32bit code

; this is where we will start
; first check if the loader did a good job
GLOBAL entry
entry:

   ; we get these from grub
   ;
   ; until paging is properly set up, all addresses are "corrected" using the "xx - BASE" - construct
   ; xx is a virtual address above 2GB, BASE is the offset to subtract to get the actual physical address.
   ;
   mov [multi_boot_magic_number-BASE], eax;
   mov [multi_boot_structure_pointer-BASE], ebx;


   mov edi,0B8000h; load frame buffer start
   mov ecx,0B8FA0h; end of frame buffer
   sub ecx, edi ; how much data do we have to clear
   xor eax, eax ; we want to fill with 0
   rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0



   mov word[0B8000h], 9F30h ; show something on screen just in case we get stuck, so that we know where


   mov eax,[ds_magic - BASE] ; value of memory pointed to by ds_magic symbol into eax
   cmp eax, DATA_SEGMENT_MAGIC

   ; comment this next statement to just see a few characters on the screen, really impressive :)
   je data_segment_ok ; if its the same then everything is good

; if we end up here the data segment is _not_ ok
; write something to the frame buffer and halt

   mov word[0B8000h], 9F44h
   mov word[0B8002h], 9F61h
   mov word[0B8004h], 9F74h
   mov word[0B8006h], 9F61h
   mov word[0B800Ah], 9F45h
   mov word[0B800Ch], 9F72h
   mov word[0B800Eh], 9F72h
   mov word[0B8010h], 9F6fh
   mov word[0B8012h], 9F72h

   jmp short $ ; now what does this do? it just jumps to itself looping until the end of all times

data_segment_ok:

    mov word[0B8002h], 9F31h ; show something on screen just in case we get stuck, so that we know where

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

   mov edi,bss_start_address - BASE; load bss address
   mov ecx, bss_end_address - BASE; end of bss and stack (!), this symbol is at the very end of the kernel
   sub ecx, edi ; how much data do we have to clear
   xor eax, eax ; we want to fill with 0
   rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

   ; setup the stack pointer to point to our stack in the just cleared bss section
    mov esp,stack - BASE

   push dword 2
   popf

    mov word[0B8004h], 9F32h


	EXTERN parseMultibootHeader;

	mov eax,parseMultibootHeader - BASE
	call eax

	EXTERN initialiseBootTimePaging

	mov eax,initialiseBootTimePaging - BASE
	call eax

    mov word[0B8006h], 9F33h

	; prepare paging, set CR3 register to start of page directory

	mov     eax,kernel_page_directory_start - BASE; eax = &PD
	mov     cr3,eax         ; cr3 = &PD


	; set bit 0x4 to 1 in cr4 to enable PSE
	; need a pentium cpu for this but it will give us 4mbyte pages
	mov eax,cr4;
	or eax, 0x00000010;
	mov cr4,eax;

	;  2) setting CR0's PG bit to enable paging

    mov word[0B8008h], 9F34h


	mov     eax,cr0
	or      eax,0x80010001   ; Set PG bit
	mov     cr0,eax         ; Paging is on!

; now paging is on, we no longer need the -BASE-correction since everything is now done using pageing!

    mov word[0B800Ah], 9F35h

 	mov esp,stack



    mov word[0B800Ch], 9F36h

  EXTERN startup ; tell the assembler we have a main somewhere
	mov eax, startup
	call eax ; jump to C
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
GLOBAL mboot
mboot:
   dd MULTIBOOT_HEADER_MAGIC
   dd MULTIBOOT_HEADER_FLAGS
   dd MULTIBOOT_CHECKSUM
   dd 0 ; mode 
   dd 800 ;width
   dd 600 ; height
   dd 32; depth

section .text
; this is the data section
; you can see that the first thing we have here is our magic value

GLOBAL diediedie
diediedie:		jmp $

SECTION .gdt_stuff
; have a look at http://www.intel.com/Assets/ja_JP/PDF/manual/253668.pdf
gdt:

; this is the first global descriptor table
; this stuff is used for segmenting, we dont need anything special
; here to just boot and present some things



; NULL descriptor
	dw 0			; limit 15:0
	dw 0			; base 15:0
	db 0			; base 23:16
	db 0			; type (3 bits for the type + system/code or data, privilege level and present bit)
	db 0			; limit 19:16, flags
	db 0			; base 31:24

; unused descriptor
	dw 0
	dw 0
	db 0
	db 0
	db 0
	db 0


; the first interesting descriptor; this one is for data

   LINEAR_DATA_SEL	equ	$-gdt
	dw 0FFFFh ; the limit, since the page-granular bit is turned on this is shifted 12 bits to the left
             ; in our case this means that the segment spawns the whole 32bit address space
	dw 0	 ; since it also starts at 0
	db 0
	db 92h	 ; 1 00 1 0010 == present, ring 0, data, expand-up, writable
	db 0CFh  ; 1 1 0 0 granularity:page (4 gig limit), 32-bit, not a 64 bit code segment
	db 0


; basically the same thing as before, but we also need a descriptor for code
   LINEAR_CODE_SEL	equ	$-gdt
	dw 0FFFFh
	dw 0
	db 0
	db 9Ah			; 1 00 1 1010 present,ring 0,code,execute,readable
	db 0CFh         ; page-granular (4 gig limit), 32-bit
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
	db 0FAh			; present,ring 3,code,execute, readable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0
  gdt_end:

; dummy TSS Segment Descriptor
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

SECTION .data
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
   ; here we create lots of room for our stack: 16 kiB (actually this is done by the resd 4096 )
   GLOBAL stack_start
stack_start:
   resd 4096
   GLOBAL stack
stack:
SECTION .paging_stuff
GLOBAL kernel_page_directory_start
kernel_page_directory_start:
%rep 1024
  dd 0
%endrep
GLOBAL kernel_page_tables_start:
kernel_page_tables_start:
%rep 1024
  dd 0
%endrep
%rep 1024
  dd 0
%endrep
%rep 1024
  dd 0
%endrep
%rep 1024
  dd 0
%endrep
