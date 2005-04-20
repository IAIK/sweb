;
; $Id: boot.s,v 1.4 2005/04/20 09:00:28 nomenquis Exp $
;
; $Log: boot.s,v $
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
MULTIBOOT_AOUT_KLUDGE   equ 1<<16
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE
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

EXTERN initialiseBootTimePaging

mov eax,initialiseBootTimePaging - BASE
call eax



mov     eax,(1024*1024*4); eax = &PD
mov     cr3,eax         ; cr3 = &PD


; set bit 0x4 to 1 in cr4 to enable PSE
; need a pentium cpu for this but it will give us 4mbyte pages
mov eax,cr4;
or eax, 0x00000010;
mov cr4,eax;

;  2) setting CR0's PG bit.

mov     eax,cr0
or      eax,0x80000001   ; Set PG bit
mov     cr0,eax         ; Paging is on!

jmp     $ + 2          ; Flush the instruction queue.





 mov edi,bss_start_address - BASE; load bss address
 mov ecx, bss_end_address - BASE ; end of bss and stack (!), this symbol is at the very end of the kernel
 sub ecx, edi ; how much data do we have to clear
 xor eax, eax ; we want to fill with 0
 rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

 mov esp,stack




mov eax, PagingMode
call eax


PagingMode:


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






EXTERN removeBootTimeIdentMapping

call removeBootTimeIdentMapping



mov     eax,(1024*1024*4); eax = &PD
mov     cr3,eax         ; cr3 = &PD



   ; set up interrupt handlers, then load IDT register
   mov ecx,(idt_end - idt) >> 3 ; number of exception handlers
   mov edi,idt
   mov esi,isr0
do_idt:
   mov eax,esi			; EAX=offset of entry point
   mov [edi],ax			; set low 16 bits of gate offset
   shr eax,16
   mov [edi + 6],ax		; set high 16 bits of gate offset
   add edi,8			; 8 bytes/interrupt gate
   add esi,(isr1 - isr0)		; bytes/stub
   loop do_idt

   mov eax, idt_ptr

   lidt [eax]


;call inivalidate_ident_mapping
;call inivalidate_ident_mapping

;writeTestOnScreen

EXTERN panic

;push  0
;call panic



; GRUB 0.90 leaves the NT bit set in EFLAGS. The first IRET we attempt
; will cause a TSS-based task-switch, which will cause Exception 10.
; Let's prevent that:



   push dword 2
   popf

EXTERN main ; tell the assembler we have a main somewhere

   call main ; hellloooo, here we are in c !
   jmp $ ; suicide

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;,
;; this will help us to boot, this way we can tell grub
;; what to do

ALIGN 4
mboot:
   dd MULTIBOOT_HEADER_MAGIC
   dd MULTIBOOT_HEADER_FLAGS
   dd MULTIBOOT_CHECKSUM
; aout kludge. These must be PHYSICAL addresses
   dd mboot - BASE
   dd text_start_address - BASE
   dd bss_start_address  - BASE
   dd kernel_end_address - BASE
   dd entry - BASE


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; interrupt/exception handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXTERN handleInterrupt;

; I shouldn't have to do this!
%macro PUSHB 1
   db 6Ah
   db %1
%endmacro

%macro INTR 1        ; (byte offset from start of stub)
GLOBAL isr%1
isr%1:
   push byte 0       ; ( 0) fake error code
   PUSHB %1          ; ( 2) exception number
   push gs           ; ( 4) push segment registers
   push fs           ; ( 6)
   push es           ; ( 8)
   push ds           ; ( 9)
   pusha             ; (10) push GP registers
      mov ax,LINEAR_DATA_SEL; (11) put known-good values...
      mov ds,eax     ; (15) ...in segment registers
      mov es,eax     ; (17)
      mov fs,eax     ; (19)
      mov gs,eax     ; (21)
      mov eax,esp    ; (23)
      push eax       ; (25) push pointer to regs_t
.1:
; setvect() changes the operand of the CALL instruction at run-time,
; so we need its location = 27 bytes from start of stub. We also want
; the CALL to use absolute addressing instead of EIP-relative, so:
      mov eax,handleInterrupt; (26)
      call eax       ; (31)
      jmp all_ints   ; (33)
%endmacro            ; (38)


%macro INTR_EC 1
GLOBAL isr%1
isr%1:
	nop				; error code already pushed
	nop				; nop+nop=same length as push byte
	PUSHB %1			; ( 2) exception number
	push gs				; ( 4) push segment registers
	push fs				; ( 6)
	push es				; ( 8)
	push ds				; ( 9)
	pusha				; (10) push GP registers
		mov ax,LINEAR_DATA_SEL	; (11) put known-good values...
		mov ds,eax		; (15) ...in segment registers
		mov es,eax		; (17)
		mov fs,eax		; (19)
		mov gs,eax		; (21)
		mov eax,esp		; (23)
		push eax		; (25) push pointer to regs_t
.1:
; setvect() changes the operand of the CALL instruction at run-time,
; so we need its location = 27 bytes from start of stub. We also want
; the CALL to use absolute addressing instead of EIP-relative, so:
			mov eax,handleInterrupt; (26)
			call eax	; (31)
			jmp all_ints	; (33)
%endmacro				; (38)

; the vector within the stub (operand of the CALL instruction)
; is at (isr0.1 - isr0 + 1)

all_ints:
	pop eax
	popa				; pop GP registers
	pop ds				; pop segment registers
	pop es
	pop fs
	pop gs
	add esp,8			; drop exception number and error code
	iret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			getvect
; action:		reads interrupt vector
; in:			[EBP + 12] = vector number
; out:			vector stored at address given by [EBP + 8]
; modifies:		EAX, EDX
; minimum CPU:		'386+
; notes:		C prototype:
;			typedef struct
;			{	unsigned access_byte, eip; } vector_t;
;			getvect(vector_t *v, unsigned vect_num);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GLOBAL getvect
getvect: 
	push ebp
		mov ebp,esp
		push esi
		push ebx
			mov esi,[ebp + 8]

; get access byte from IDT[i]
			xor ebx,ebx
			mov bl,[ebp + 12]
			shl ebx,3
			mov al,[idt + ebx + 5]
			mov [esi + 0],eax

; get handler address from stub
			mov eax,isr1
			sub eax,isr0	; assume stub size < 256 bytes
			mul byte [ebp + 12]
			mov ebx,eax
			add ebx,isr0
			mov eax,[ebx + (isr0.1 - isr0 + 1)]
			mov [esi + 4],eax
		pop ebx
		pop esi
	pop ebp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			setvect
; action:		writes interrupt vector
; in:			[EBP + 12] = vector number,
;			vector stored at address given by [EBP + 8]
; out:			(nothing)
; modifies:		EAX, EDX
; minimum CPU:		'386+
; notes:		C prototype:
;			typedef struct
;			{	unsigned access_byte, eip; } vector_t;
;			getvect(vector_t *v, unsigned vect_num);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GLOBAL setvect
setvect: 
	push ebp
		mov ebp,esp
		push esi
		push ebx
			mov esi,[ebp + 8]

; store access byte in IDT[i]
			mov eax,[esi + 0]
			xor ebx,ebx
			mov bl,[ebp + 12]
			shl ebx,3
			mov [idt + ebx + 5],al

; store handler address in stub
			mov eax,isr1
			sub eax,isr0	; assume stub size < 256 bytes
			mul byte [ebp + 12]
			mov ebx,eax
			add ebx,isr0
			mov eax,[esi + 4]
			mov [ebx + (isr0.1 - isr0 + 1)],eax
		pop ebx
		pop esi
	pop ebp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; interrupt/exception stubs
; *** CAUTION: these must be consecutive, and must all be the same size.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INTR 0		; zero divide (fault)
	INTR 1		; debug/single step
	INTR 2		; non-maskable interrupt (trap)
	INTR 3		; INT3 (trap)
	INTR 4		; INTO (trap)
	INTR 5		; BOUND (fault)
	INTR 6		; invalid opcode (fault)
	INTR 7		; coprocessor not available (fault)
	INTR_EC 8	; double fault (abort w/ error code)
	INTR 9		; coproc segment overrun (abort; 386/486SX only)
	INTR_EC 0Ah	; bad TSS (fault w/ error code)
	INTR_EC 0Bh	; segment not present (fault w/ error code)
	INTR_EC 0Ch	; stack fault (fault w/ error code)
	INTR_EC 0Dh	; GPF (fault w/ error code)
	INTR_EC 0Eh	; page fault
	INTR 0Fh	; reserved
	INTR 10h	; FP exception/coprocessor error (trap)
	INTR 11h	; alignment check (trap; 486+ only)
	INTR 12h	; machine check (Pentium+ only)
	INTR 13h
	INTR 14h
	INTR 15h
	INTR 16h
	INTR 17h
	INTR 18h
	INTR 19h
	INTR 1Ah
	INTR 1Bh
	INTR 1Ch
	INTR 1Dh
	INTR 1Eh
	INTR 1Fh

; isr20 through isr2F are hardware interrupts. The 8259 programmable
; interrupt controller (PIC) chips must be reprogrammed to make these work.
	INTR 20h	; IRQ 0/timer interrupt
	INTR 21h	; IRQ 1/keyboard interrupt
	INTR 22h
	INTR 23h
	INTR 24h
	INTR 25h
	INTR 26h	; IRQ 6/floppy interrupt
	INTR 27h
	INTR 28h	; IRQ 8/real-time clock interrupt
	INTR 29h
	INTR 2Ah
	INTR 2Bh
	INTR 2Ch
	INTR 2Dh	; IRQ 13/math coprocessor interrupt
	INTR 2Eh	; IRQ 14/primary ATA ("IDE") drive interrupt
	INTR 2Fh	; IRQ 15/secondary ATA drive interrupt

; syscall software interrupt
	INTR 30h

; the other 207 vectors are undefined

%assign i 31h
%rep (0FFh - 30h)

	INTR i

%assign i (i + 1)
%endrep


; this is the data section
; you can see that the first thing we have here is our magic value

GLOBAL diediedie
diediedie:
jmp $

SECTION .data



; this descritor is _REALLY_ braindead
; have a look at http://www.csee.umbc.edu/~plusquel/310/slides/micro_arch2.html

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

gdt_ptr:
	dw gdt_end - gdt - 1
	dd gdt - BASE

gdt_ptr_new:

	dw gdt_end - gdt - 1
	dd gdt
	


%rep 1024
 dd 0
%endrep

; 256 ring 0 interrupt gates
; we certainly miss one ring 3 interrupt gate (for syscalls)
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


SECTION .bss
   ; here we create lots of room for our stack (actually this is one by the resd 1024)
   resd 65536
stack:
 
 SECTION .bss
 kernel_page_directory_start:
  resd 4096
