# Startup code for APs

# TODO: Pass these as arguments from the BSP instead of hardcoding
.equ KERNEL_CS, 0x10
.equ KERNEL_DS, 0x20

.code16gcc
.section .text.apstartup, "x"

.macro ap_print string
    movl $\string, %ebx
1:
    movb (%ebx), %al
    testb %al, %al
    jz 2f
	movw $0xE9, %dx
    outb %al, %dx
    addl $1, %ebx
    jmp 1b
2:
.endm

.macro ap_print64 string
    movq $\string, %rbx
1:
    movb (%rbx), %al
    testb %al, %al
    jz 2f
    movw $0xE9, %dx
    outb %al, %dx
    addq $1, %rbx
    jmp 1b
2:
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

# APs start in real mode and directly enter long mode without going to protected mode first
.global apstartup
apstartup:
    cli

    movw %cs, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    ap_print (ap_s_start - apstartup)

    ap_print (ap_s_pae - apstartup)

    # Enable PAE
	movl %cr4, %eax
    orl  $0x20,%eax
    movl %eax, %cr4

    ap_print (ap_s_cr3 - apstartup)

    # Load CR3
    movl $(ap_kernel_cr3 - apstartup), %eax
    movl (%eax), %eax
	movl %eax, %cr3

    ap_print (ap_s_efer - apstartup)

    # Enable EFER.LME and EFER.NXE
    mov $0xC0000080, %ecx
    rdmsr
    orl $0x900, %eax
    wrmsr

    ap_print (ap_s_prot - apstartup)

    # Enable protected mode, write protection in kernel mode and paging
    movl %cr0, %eax
    orl  $0x80010001,%eax
    movl %eax, %cr0

    ap_print (ap_s_lgdt - apstartup)

    movl $(ap_gdt32_ptr - apstartup), %eax
    lgdt (%eax)

    ap_print (ap_s_ljmp - apstartup)

    ljmp $KERNEL_CS, $(apstartup64_trampoline - apstartup)
    hlt

.code64
.global apstartup64_trampoline
apstartup64_trampoline:
    ap_print64 ap_s_apstartup64

.extern apstartup 64
    movq $apstartup64, %rax
    jmp *%rax
    hlt

ap_s_start:
    .asciz "AP started\n"
ap_s_pae:
	.asciz "Enabling PAE\n"
ap_s_cr3:
	.asciz "Loading CR3\n"
ap_s_efer:
	.asciz "Enabling EFER.LME and EFER.NXE\n"
ap_s_prot:
	.asciz "Enabling protected mode, write protection in kernel mode and paging\n"
ap_s_lgdt:
	.asciz "Loading GDT\n"
ap_s_segm:
	.asciz "Loading segment registers\n"
ap_s_ljmp:
	.asciz "Far jmp to trampoline to load long mode code segment\n"
ap_s_apstartup64:
	.asciz "Jumping to apstartup64\n"

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



.code64
