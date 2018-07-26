# Startup code for APs

.code16gcc

.section .text.apstartup, "x"
.global apstartup
apstartup:
    cli

    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $0x41, %al
    mov $0xE9, %dx

loop:
    outb %al, %dx
    #jmp loop
    hlt

    #lgdt ap_gdt_ptr

ap_gdt_ptr:
.global ap_gdt_ptr
    .word 0xf2
    .long 0x800
ap_gdt_ptr_end:

.code64
