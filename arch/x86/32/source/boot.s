
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

%macro writeTestOnScreen 0
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
   jmp short $ ; now what does this do? it just jumps to this instruction until the end of all times
%endmacro

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
   ;mov ax,LINEAR_CORE0_SEL
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

    mov word[0B8004h], 9F32h


    ; ----------------------get mc info out of ft and mp table----------------------

    ;parse multi process info
    EXTERN parseMPTable;
    mov eax,parseMPTable - BASE
    call eax

    ; ----------------------get mc info out of ft and mp table END----------------------


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
; the following memory-clearing should already have been covered by the generic .bss-cleaning above,
; since the stack is part of the .bss
 	;mov edi,stack_start; load bss address
 	;mov ecx, stack; end of bss and stack (!), this symbol is at the very end of the kernel
 	;sub ecx, edi ; how much data do we have to clear
 	;xor eax, eax ; we want to fill with 0
 	;rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

 	mov esp,stack



    mov word[0B800Ch], 9F36h

	;; UNKLAR wozu das nötig ist? Aber ohne gehts net .. .:) Dokumentieren wär gut
	mov eax, PagingMode
	call eax


PagingMode:

   mov word[0C00B800Eh], 9F38h

   ; this has already been done above and not undone so removed PL

   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space

   ;lgdt [gdt_ptr_new]

   ; now prepare all the segment registers to use our segments
   ;mov ax, LINEAR_DATA_SEL
   ;mov ds,ax
   ;mov es,ax
   ;mov ss,ax
   ;mov fs,ax
   ;mov gs,ax

   ; use these segments
   ;jmp LINEAR_CODE_SEL:(now_using_segments_new)

now_using_segments_new:

   mov word[0C00B8010h], 9F39h



EXTERN removeBootTimeIdentMapping

call removeBootTimeIdentMapping


   mov word[0C00B8012h], 4330h

; me thinks this has already been done, so why again
; mov     eax,kernel_page_directory_start - BASE; eax = &PD
; mov     cr3,eax         ; cr3 = &PD

   mov word[0C00B8014h], 4331h


   mov word[0C00B801Ah], 4335h


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
   mov ax,LINEAR_CORE0_SEL
   mov gs,ax

   ; use these segments
   jmp LINEAR_CODE_SEL:(reload_segments_new)

reload_segments_new:

    ret;
    
section .mc_text
BITS 16 ; we want 16bit code
global ap_startup
ap_startup:
   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space

    cli

	mov ax, cs
	mov ds, ax

    lgdt [gdt_ptr2 - ap_startup]

	mov     eax,0x0001   	; Set PE bit
	mov     cr0,eax         ; Protected mode enabled

    mov eax, LINEAR_CORE1_SEL

    mov ax, LINEAR_DATA_SEL
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax

   ; use these segments
   jmp LINEAR_CODE_SEL:(protected_mode_bootup_ap - ap_startup)

; workaround because AP can only load from low memory right now
; at the address 0x800 actually is the GDT because be coppied it before
gdt_ptr2:
	dw 0x2f
	dd 0x800
gdt_ptr2_end:

BITS 32 ; we want 32bit code
global set_cpu_registers_for_ap
protected_mode_bootup_ap:

	mov     eax,kernel_page_directory_start - LINK_BASE + LOAD_BASE	; eax = &PD
	mov     cr3,eax         										; cr3 = &PD


	; set bit 0x4 to 1 in cr4 to enable PSE
	; need a pentium cpu for this but it will give us 4mbyte pages
	mov eax,cr4;
	or eax, 0x00000010;
	mov cr4,eax;

	mov     eax,cr0
	or      eax,0x80010001   	; Set PG bit
	mov     cr0,eax         	; Paging is on!
	; now paging is on, we no longer need the -LINK_BASE-correction since everything is now done using pageing!

 	mov esp,stack
 	
 	; The NT flag is set at this point, therefore the first IRET we attempt
	; will cause a TSS-based (nested) task-switch, which will cause Exception 10.
	; Let's prevent that:
	; push new EFLAGS value (NT bit == bit number 14 NOT set) to the stack and
	; load one dword from the stack into the EFLAGS
 	push dword 2
    popf

	EXTERN ap_complete_startup
    mov eax, ap_complete_startup
    call eax

    jmp $

global ap_startup_end
ap_startup_end:

global reload_segments_core1
reload_segments_core1:
   ; ok, next thing to do is to load our own descriptor table
   ; this one will spawn just one huge segment for the whole address space
   lgdt [gdt_ptr_very_new]
   
   

   ; now prepare all the segment registers to use our segments
   mov ax, LINEAR_DATA_SEL
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov ax,LINEAR_CORE1_SEL
   mov gs,ax
   
   

   ; use these segments
   jmp LINEAR_CODE_SEL:(reload_segments_new)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;,
;; this will help us to boot, this way we can tell grub
;; what to do

SECTION .mboot
GLOBAL mboot
mboot:
   dd MULTIBOOT_HEADER_MAGIC
   dd MULTIBOOT_HEADER_FLAGS
   dd MULTIBOOT_CHECKSUM
; aout kludge. These must be PHYSICAL addresses
;   dd mboot - BASE
;   dd text_start_address - BASE
;   dd bss_start_address  - BASE
;   dd kernel_end_address - BASE
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
diediedie:		jmp $

SECTION .gdt_stuff
; have a look at http://www.intel.com/Assets/ja_JP/PDF/manual/253668.pdf
GLOBAL gdt
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
	
; the first interesting descriptor; this one is for data

; TODO: adresse jetzt: 80 07 9000 --> local_core0_storage

;global core_local_selector
;core_local_selector:
;   LINEAR_CORE0_SEL  equ $-gdt
;    dw 01h  ; the limit, since the page-granular bit is turned on this is shifted 12 bits to the left
;               ; in our case this means that the segment spawns the whole 32bit address space
;    dw 9000h   ; since it also starts at 0
;    db 07h
;    db 92h   ; 1 00 1 0010 == present, ring 0, data, expand-up, writable
;    db 0CFh  ; 1 1 0 0 granularity:page (4 gig limit), 32-bit, not a 64 bit code segment
;    db 80h
    
;   LINEAR_CORE1_SEL  equ $-gdt
;    dw 01h  ; the limit, since the page-granular bit is turned on this is shifted 12 bits to the left
               ; in our case this means that the segment spawns the whole 32bit address space
;    dw 0A000h   ; since it also starts at 0
;    db 07h
;    db 92h   ; 1 00 1 0010 == present, ring 0, data, expand-up, writable
;    db 0CFh  ; 1 1 0 0 granularity:page (4 gig limit), 32-bit, not a 64 bit code segment
;    db 80h
    
global core_local_selector
core_local_selector:
   LINEAR_CORE0_SEL  equ $-gdt
	dw 0
	dw 0
	db 0
	db 0
	db 0
	db 0
    
   LINEAR_CORE1_SEL  equ $-gdt
	dw 0
	dw 0
	db 0
	db 0
	db 0
	db 0
GLOBAL gdt_end
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

; dummy TSS Segment Descriptor for 2nd Core
global tss_selector2
tss_selector2:
 TSS_SEL2	equ	$-gdt
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

gdtr:
    dw 00
    dd 00

  
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
GLOBAL cpu_info_pointer
cpu_info_pointer:
	dd 0

GLOBAL mp_table
mp_table:
	dd 0
   
SECTION .bss
   ; here we create lots of room for our stack: 16 kiB (actually this is done by the resd 4096 )
GLOBAL stack_start
stack_start:
   resd 4096
   GLOBAL stack
stack:
   resd 1024
GLOBAL core0_local_storage
core0_local_storage:
   resd 1024
GLOBAL core1_local_storage
core1_local_storage:

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
