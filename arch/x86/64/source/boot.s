BASE             EQU     0FFFFFFFF80000000h
PHYS_BASE        EQU     0FFFFFFFF00000000h

EXTERN boot_stack

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
GLOBAL tss
tss:
 dw 0h ; segment limit
GLOBAL tss.base_low
.base_low:
 dw 0 ; base
GLOBAL tss.base_high_word_low
.base_high_word_low:
 db 0 ; base
 db 089h  ; 1000 1010 == present, ring 0, system segment, code, expand-up, writable, not accessed
 db 0a0h  ; 1000 0000 == granularity:page, not default, 64 bit, not avl, segment limit (4 bits)
GLOBAL tss.base_high_word_high
.base_high_word_high:
 db 0
 dd 0ffffffffh
 dd 0
gdt_end:

GLOBAL gdt_ptr
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
