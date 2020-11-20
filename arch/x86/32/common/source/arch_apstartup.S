# Startup code for APs

# TODO: Pass these as arguments from the BSP instead of hardcoding
.equ KERNEL_CS, 0x10
.equ KERNEL_DS, 0x20

.code16gcc
.section .text.apstartup, "x"

.macro ap_print string
    movl $\string, %esi
    movw $0xE9, %dx
1:
    lodsb
    testb %al, %al
    jz 2f
    outb %al, %dx
    jmp 1b
2:
    .endm

.macro ap_printn string len
    movl $\string, %esi
    movl $\len, %ecx
    movw $0xE9, %dx
    rep outsb
.endm


.macro ap_dump addr num
    movl $\addr, %ebx
    movl $\num, %ecx
1:
    testl %ecx, %ecx
    jz 2f

    movb (%ebx), %al
    movw $0xE9, %dx
    outb %al, %dx
    addl $1, %ebx
    subl $1, %ecx
    jmp 1b
2:
.endm

# APs start in real mode
.global apstartup
apstartup:
    cli

    movw %cs, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

#    ap_print (ap_s_start - apstartup)
    ap_printn (ap_s_start - apstartup), len_ap_s_start

#    ap_print (ap_s_pse - apstartup)
    ap_printn (ap_s_pse - apstartup), len_ap_s_pse

    # Enable PSE
	movl %cr4, %eax
    orl  $0x10,%eax
    movl %eax, %cr4

#    ap_print (ap_s_cr3 - apstartup)
    ap_printn (ap_s_cr3 - apstartup), len_ap_s_cr3

    # Load CR3
    movl $(ap_kernel_cr3 - apstartup), %eax
    movl (%eax), %eax
	movl %eax, %cr3

#    ap_print (ap_s_prot - apstartup)
    ap_printn (ap_s_prot - apstartup), len_ap_s_prot

    # Enable protected mode, write protection in kernel mode and paging
    movl %cr0, %eax
    orl  $0x80010001,%eax
    movl %eax, %cr0

#    ap_print (ap_s_lgdt - apstartup)
    ap_printn (ap_s_lgdt - apstartup), len_ap_s_lgdt

    movl $(ap_gdt32_ptr - apstartup), %eax
    lgdt (%eax)

#    ap_print (ap_s_apstartup32 - apstartup)
    ap_printn (ap_s_apstartup32 - apstartup), len_ap_s_apstartup32
    movl $apstartup32, %eax
	jmp *%eax

#    ljmp $KERNEL_CS, $(apstartup32_trampoline - apstartup)
    hlt

.code32
.global apstartup32_trampoline
apstartup32_trampoline:
#    ap_print (ap_s_apstartup32 - apstartup)
    ap_printn (ap_s_apstartup32 - apstartup), len_ap_s_apstartup32

.extern apstartup32
    movl $apstartup32, %eax
    jmp *%eax
    hlt

.global ap_s_start
ap_s_start:
    .ascii "AP started\n"
    .equ len_ap_s_start, . - ap_s_start
.global ap_s_pse
ap_s_pse:
	.ascii "Enabling PSE\n"
    len_ap_s_pse = . - ap_s_pse
.global ap_s_cr3
ap_s_cr3:
	.ascii "Loading CR3\n"
    .equ len_ap_s_cr3, . - ap_s_cr3
.global ap_s_prot
ap_s_prot:
	.ascii "Enabling protected mode, write protection in kernel mode and paging\n"
    len_ap_s_prot = . - ap_s_prot
.global ap_s_lgdt
ap_s_lgdt:
	.ascii "Loading GDT\n"
    .equ len_ap_s_lgdt, . - ap_s_lgdt
.global ap_s_segm
ap_s_segm:
	.ascii "Loading segment registers\n"
    .equ len_ap_s_segm, . - ap_s_segm
.global ap_s_ljmp
ap_s_ljmp:
	.ascii "Far jmp to trampoline to load long mode code segment\n"
    .equ len_ap_s_ljmp, . - ap_s_ljmp
.global ap_s_apstartup32
ap_s_apstartup32:
	.ascii "Jumping to apstartup32\n"
    .equ len_ap_s_apstartup32, . - ap_s_apstartup32

# To be filled in by the BSP
.global ap_kernel_cr3
ap_kernel_cr3:
    .skip 4

.global ap_gdt32_ptr
ap_gdt32_ptr:
    .skip 2	# uint16 limit
    .skip 4	# uint32 addr

.global ap_gdt32
ap_gdt32:
    .skip 0x70


.align 0x1000
.global ap_pml4
ap_pml4:
.skip 0x1000



.code32
