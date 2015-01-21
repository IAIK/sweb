BASE             EQU     0FFFFFFFF80000000h         ; Base address (virtual) (kernel is linked to start at this address)
PHYS_BASE        EQU     0FFFFFFFF00000000h

EXTERN text_start_address, text_end_address,bss_start_address, bss_end_address, kernel_end_address

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

  mov [multi_boot_structure_pointer-BASE], ebx;

  mov edi,0B8000h; load frame buffer start
  mov ecx,0B8FA0h; end of frame buffer
  sub ecx, edi ; how much data do we have to clear
  xor eax, eax ; we want to fill with 0
  rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

  ; next thing to do to be c compliant is to
  ; clear the bss (uninitialised data)

  mov edi,bss_start_address - BASE; load bss address
  mov ecx, bss_end_address - BASE; end of bss and stack (!), this symbol is at the very end of the kernel
  sub ecx, edi ; how much data do we have to clear
  xor eax, eax ; we want to fill with 0
  rep stosb ;  Fill (E)CX bytes at ES:[(E)DI] with AL, in our case 0

  EXTERN boot_stack
  ; setup the stack pointer to point to our stack in the just cleared bss section
  mov esp,boot_stack + 0x4000 - BASE
  mov ebp,esp

  call _initialiseBootTimePaging

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

; GRUB 0.90 leaves the NT bit set in EFLAGS. The first IRET we attempt
; will cause a TSS-based task-switch, which will cause Exception 10.
; Let's prevent that:

  push dword 2
  popf

  ;  2) setting CR0's PG bit to enable paging

  mov     eax,cr0         ; Set PG bit
  or eax,0x80000001
  mov     cr0,eax         ; Paging is on!

  mov eax, g_tss - PHYS_BASE
  mov word[tss.base_low - BASE], ax
  shr eax, 16
  mov byte[tss.base_high_word_low - BASE], al
  shr eax, 8
  mov byte[tss.base_high_word_high - BASE], al

  lgdt [gdt_ptr - BASE]

  EXTERN entry64
  jmp LINEAR_CODE_SEL:(entry64-BASE)
EXTERN _entry
call _entry
  hlt

global _initialiseBootTimePaging
_initialiseBootTimePaging:
  EXTERN kernel_page_map_level_4
  EXTERN kernel_page_directory_pointer_table
  EXTERN kernel_page_directory
  mov dword[kernel_page_map_level_4 - BASE + 0], $kernel_page_directory_pointer_table - BASE + 3
  mov dword[kernel_page_map_level_4 - BASE + 4], 0
  mov dword[kernel_page_directory_pointer_table - BASE + 0], $kernel_page_directory - BASE + 3
  mov dword[kernel_page_directory_pointer_table - BASE + 4], 0
  mov dword[kernel_page_directory - BASE + 0], 083h
  mov dword[kernel_page_directory - BASE + 4], 0
  ret

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

GLOBAL gdt_ptr_new
gdt_ptr_new:
 dw gdt_end - gdt - 1
 dq gdt

GLOBAL g_tss
g_tss:
   dd 0 ; reserved
   dd boot_stack + 0x4000 - PHYS_BASE ; rsp0 lower
   dd 0FFFFFFFFh ; rsp0 higher
   dd 0 ; rsp1 lower
   dd 0 ; rsp1 higher
   dd 0 ; rsp2 lower
   dd 0 ; rsp2 higher
   dd 0 ; reserved
   dd 0 ; reserved
   dd boot_stack + 0x4000 - PHYS_BASE ; ist1 lower
   dd 0FFFFFFFFh ; ist1 higher
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dd 0
   dw 0
   dw 0

SECTION .data
BITS 32
GLOBAL multi_boot_structure_pointer
multi_boot_structure_pointer:
  dd 0
