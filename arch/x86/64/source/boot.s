LINK_BASE             EQU     0FFFFFFFF80000000h         ; Base address (virtual) (kernel is linked to start at this address)
PHYS_BASE             EQU     0FFFFFFFF00000000h
LOAD_BASE             EQU     00000000h                  ; Base address (physikal) (kernel is actually loaded at this address)
BASE                  EQU     (LINK_BASE - LOAD_BASE)    ; difference to calculate physical adress from virtual address until paging is set up

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

   ; from http://wiki.osdev.org/User:Stephanvanschaik/Setting_Up_Long_Mode
   mov eax, 0x80000000    ; Set the A-register to 0x80000000.
   cpuid                  ; CPU identification.
   cmp eax, 0x80000001    ; Compare the A-register with 0x80000001.
   jb NoLongMode

   mov eax, 0x80000001    ; Set the A-register to 0x80000001.
   cpuid                  ; CPU identification.
   test edx, 1 << 29      ; Test if the LM-bit, which is bit 29, is set in the D-register.
   jz NoLongMode          ; They aren't, there is no long mode.

   ;test edx, 1 << 26      ; check for 1 GiB paging
   ;jz No1GBMode
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

   hlt

NoLongMode:
   mov word[0B8000h], 9F4Eh
   mov word[0B8002h], 9F4Fh
   mov word[0B8006h], 9F4Ch
   mov word[0B8008h], 9F4Fh
   mov word[0B800Ah], 9F4Eh
   mov word[0B800Ch], 9F47h
   mov word[0B8010h], 9F4Dh
   mov word[0B8012h], 9F4Fh
   mov word[0B8014h], 9F44h
   mov word[0B8016h], 9F45h

   hlt

data_segment_ok:

    mov word[0B8002h], 9F31h ; show something on screen just in case we get stuck, so that we know where

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

  pop eax

  call initialiseBootTimePaging

  ; set bit 0x4 to 1 in cr4 to enable PSE
  ; need a pentium cpu for this but it will give us 4mbyte pages
  mov eax,cr4;
  bts eax, 0x4;
  bts eax, 0x5;
  mov cr4,eax;

  mov     eax,kernel_page_map_level_4 - BASE; eax = &PML4
  mov     cr3,eax         ; cr3 = &PD

  ; set EFER.LME = 1 to enable long mode
  mov ecx, 0xC0000080
  rdmsr
  or eax, 0x100
  wrmsr

  mov word[0B8010h], 9F39h

; GRUB 0.90 leaves the NT bit set in EFLAGS. The first IRET we attempt
; will cause a TSS-based task-switch, which will cause Exception 10.
; Let's prevent that:

  push dword 2
  popf
  mov word[0B801Ch], 4336h

  ;  2) setting CR0's PG bit to enable paging

  mov word[0B8008h], 9F34h

  mov     eax,cr0         ; Set PG bit
  or eax,0x80000001
  mov     cr0,eax         ; Paging is on!

  mov eax, g_tss - PHYS_BASE
  mov word[tss.base_low - BASE], ax
  shr eax, 16
  mov byte[tss.base_high_word_low - BASE], al
  shr eax, 8
  mov byte[tss.base_high_word_high - BASE], al

  mov dword[g_tss.low0 - BASE], kernel_stack - PHYS_BASE
  mov dword[g_tss.high0 - BASE], 0FFFFFFFFh
  mov dword[g_tss.istl1 - BASE], kernel_stack - PHYS_BASE
  mov dword[g_tss.isth1 - BASE], 0FFFFFFFFh

  lgdt [gdt_ptr - BASE]

  jmp LINEAR_CODE_SEL:(entry64-BASE)

   hlt

global initialiseBootTimePaging
initialiseBootTimePaging:
  mov dword[kernel_page_map_level_4 - BASE + 0], $kernel_page_directory_pointer_table - BASE + 3
  mov dword[kernel_page_map_level_4 - BASE + 4], 0
  mov dword[kernel_page_directory_pointer_table - BASE + 0], $kernel_page_directory - BASE + 3
  mov dword[kernel_page_directory_pointer_table - BASE + 4], 0
  mov dword[kernel_page_directory - BASE + 0], 083h
  mov dword[kernel_page_directory - BASE + 4], 0
  ret

SECTION .text
BITS 64

; this is where we will start
; first check if the loader did a good job
GLOBAL entry64
entry64:
EXTERN parseMultibootHeader
   call parseMultibootHeader
EXTERN initialisePaging
   call initialisePaging

   mov rax,kernel_page_map_level_4 - BASE; rax = &PML4
   mov cr3,rax         ; cr3 = &PD
   mov rsp,stack
   mov rbp,stack

   mov rax, gdt_ptr_new
   lgdt [rax]
   mov rax, LINEAR_DATA_SEL
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov gs,ax
   mov rax, TSS_SEL
   ltr ax

   mov rcx,higherHalf
   jmp rcx
higherHalf:

EXTERN removeBootTimeMapping
   call removeBootTimeMapping

EXTERN startup ; tell the assembler we have a main somewhere
   call startup
   jmp $ ; suicide

SECTION .gdt_stuff
BITS 32
gdt:

; NULL descriptor
 dq 0
 dq 0
; descriptor byte 6:
;                                  Present:1
;                           PrivilegeLevel:2
;                            SystemSegment:1
;                                     Code:1
; Code Conforming/Data Expansion Direction:1
;             Readable Code/Writeable Data:1
;                                 Accessed:1
; descriptor byte 7:
;                              Granularity:1
;                                  Default:1
;                                 Reserved:1
;             Available to Sys Programmers:1
;                                    Limit:4

   LINEAR_CODE_SEL equ $-gdt
 dw 0h
 dw 0
 db 0
 db 09Ah  ; 1000 1010 == present, ring 0, no system segment, code, expand-up, writable, not accessed
 db 0A0h  ; 1010 0000 == granularity:page, not default, 64 bit, not avl, no limit set
 db 0
 dq 0

; basically the same thing as before, but we also need a descriptor for data
   LINEAR_DATA_SEL equ $-gdt
 dw 0h ; the limit, since the page-granular bit is turned on this is shifted 12 bits to the left
             ; in our case this means that the segment spawns the whole 32bit address space
 dw 0   ; since it also starts at 0
 db 0
 db 092h  ; 1000 1111 == present, ring 0, no system segment, data, expand-up, writable, not accessed
 db 0C0h  ; 1010 0000 == granularity:page, not default, 64 bit, not avl, no limit set
 db 0
 dq 0

; basically the same thing as before, but we also need a descriptor for user code
   LINEAR_USR_CODE_SEL equ $-gdt
 dw 0h
 dw 0
 db 0
 db 0FAh  ; 1111 1010 == present, ring 3, no system segment, code, expand-up, writable, not accessed
 db 0A0h  ; 1010 0000 == granularity:page, not default, 64 bit, not avl, no limit set
 db 0
 dq 0

; basically the same thing as before, but we also need a descriptor for user data
   LINEAR_USR_DATA_SEL equ $-gdt
 dw 0h ; the limit, since the page-granular bit is turned on this is shifted 12 bytes to the left
             ; in our case this means that the segment spawns the while 32bit address space
 dw 0
 db 0
 db 0F2h  ; 1110 0010 == present, ring 3, no system segment, data, expand-up, writable, not accessed
 db 0C0h  ; 1010 0000 == granularity:page, not default, 64 bit, not avl, no limit set
 db 0
 dq 0


; TSS Segment Descriptor
 TSS_SEL equ $-gdt
tss:
 dw 0h ; segment limit
.base_low:
 dw 0 ; base
.base_high_word_low:
 db 0 ; base
 db 089h  ; 1000 1010 == present, ring 0, system segment, code, expand-up, writable, not accessed
 db 0a0h  ; 1000 0000 == granularity:page, not default, 64 bit, not avl, segment limit (4 bits)
.base_high_word_high:
 db 0
 dd 0ffffffffh
 dd 0
gdt_end:

gdt_ptr:
 dw gdt_end - gdt - 1
 dd gdt - BASE

gdt_ptr_new:
 dw gdt_end - gdt - 1
 dq gdt

; 256 ring 0 interrupt gates
; we certainly miss one ring 3 interrupt gate (for syscalls)
SECTION .idt_stuff
GLOBAL idt
idt:
%rep 256
  dw 0               ; offset 15:0
  dw LINEAR_CODE_SEL ; selector
  db 0               ; (always 0 for interrupt gates)
  db 8Eh             ; present,ring 0,'386 interrupt gate 10001110
  dw 0               ; offset 31:16
  dd 0               ; offset 63:32
  dd 0               ; reserved
%endrep
idt_end:

GLOBAL idt_ptr

idt_ptr:
   dw idt_end - idt - 1    ; IDT limit
   dq idt  ; linear adr of IDT

GLOBAL g_tss
g_tss:
   dd 0 ; reserved
.low0:
   dd 0 ; rsp0 lower
.high0:
   dd 0 ; rsp0 higher
.low1:
   dd 0 ; rsp1 lower
.high1:
   dd 0 ; rsp1 higher
.low2:
   dd 0 ; rsp2 lower
.high2:
   dd 0 ; rsp2 higher
   dd 0 ; reserved
   dd 0 ; reserved
.istl1:
   dd 0 ; ist1 lower
.isth1:
   dd 0 ; ist1 higher
.istl2:
   dd 0
.isth2:
   dd 0
.istl3:
   dd 0
.isth3:
   dd 0
.istl4:
   dd 0
.isth4:
   dd 0
.istl5:
   dd 0
.isth5:
   dd 0
.istl6:
   dd 0
.isth6:
   dd 0
.istl7:
   dd 0
.isth7:
   dd 0
   dd 0
   dd 0
   dw 0
.iobase
   dw 0

SECTION .data
BITS 32
ds_magic:
   dd DATA_SEGMENT_MAGIC

SECTION .data
BITS 32
GLOBAL multi_boot_magic_number
multi_boot_magic_number:
  dd 0
GLOBAL multi_boot_structure_pointer
multi_boot_structure_pointer:
  dd 0

SECTION .bss
BITS 64
GLOBAL kernel_page_directory
kernel_page_directory:
resd 2048

GLOBAL kernel_page_directory_pointer_table
kernel_page_directory_pointer_table:
resd 2048

GLOBAL kernel_page_map_level_4
kernel_page_map_level_4:
resd 1024

GLOBAL stack_start
stack_start:
resd 4096
GLOBAL stack
stack:

GLOBAL kernel_stack_start
kernel_stack_start:
resd 4096
GLOBAL kernel_stack
kernel_stack:
