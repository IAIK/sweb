LINK_BASE           EQU     80000000h              ; Base address (virtual) (kernel is linked to start at this address)
LOAD_BASE           EQU     00100000h              ; Base address (physikal) (kernel is actually loaded at this address)
BASE                EQU     (LINK_BASE - LOAD_BASE) ; difference to calculate physical adress from virtual address until paging is set up

; this is really really bad voodoo ...
; grub needs this, or it will refuse to boot
MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_WANT_VESA equ 1<<2
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

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
