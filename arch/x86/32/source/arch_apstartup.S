# https://www.airs.com/blog/archives/518
.section .note.GNU-stack,"",@progbits

# Startup code for APs

# TODO: Pass these as arguments from the BSP instead of hardcoding
.set KERNEL_CS, 8*3
.set KERNEL_DS, 8*2

.code16gcc
.section .text.apstartup, "ax", @progbits

.macro ap_print string
    leal \string, %esi
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
    leal \string, %esi
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

    leal (ap_startup_stack - apstartup), %esp

    ap_printn (ap_s_start - apstartup), len_ap_s_start

    ap_printn (ap_s_lgdt - apstartup), len_ap_s_lgdt
ap_load_gdt:
    leal (ap_gdt32_ptr - apstartup), %eax
    lgdt (%eax)

ap_load_idt:
    lidt ap_idt_ptr

    ap_printn (ap_s_pse - apstartup), len_ap_s_pse

    # Enable PSE
	movl %cr4, %eax
    orl  $0x10,%eax
    movl %eax, %cr4

    ap_printn (ap_s_cr3 - apstartup), len_ap_s_cr3

    # Load CR3
    leal (ap_kernel_cr3 - apstartup), %eax
    movl (%eax), %eax
	movl %eax, %cr3

    ap_printn (ap_s_prot - apstartup), len_ap_s_prot

ap_prot_enable:
    # Enable protected mode, write protection in kernel mode and paging
    movl %cr0, %eax
    orl  $0x80010001,%eax
    movl %eax, %cr0

    ljmpl $KERNEL_CS, $apstartup32
    hlt
.code32

.global ap_s_start
ap_s_start:
    .ascii "AP started\n"
    .set len_ap_s_start, . - ap_s_start
.global ap_s_pse
ap_s_pse:
	.ascii "Enabling PSE\n"
    len_ap_s_pse = . - ap_s_pse
.global ap_s_cr3
ap_s_cr3:
	.ascii "Loading CR3\n"
    .set len_ap_s_cr3, . - ap_s_cr3
.global ap_s_prot
ap_s_prot:
	.ascii "Enabling protected mode, write protection in kernel mode and paging\n"
    len_ap_s_prot = . - ap_s_prot
ap_s_prot_far_jmp:
	.ascii "Far jumping to load protected mode code segment\n"
	len_ap_s_prot_far_jmp = . - ap_s_prot_far_jmp
.global ap_s_lgdt
ap_s_lgdt:
	.ascii "Loading GDT\n"
    .set len_ap_s_lgdt, . - ap_s_lgdt
.global ap_s_segm
ap_s_segm:
	.ascii "Loaded segment registers\n"
    .set len_ap_s_segm, . - ap_s_segm
.global ap_s_ljmp
ap_s_ljmp:
	.ascii "Far jmp to trampoline to load protected mode code segment\n"
    .set len_ap_s_ljmp, . - ap_s_ljmp
.global ap_s_apstartup32
ap_s_apstartup32:
	.ascii "Jumping to apstartup32\n"
    .set len_ap_s_apstartup32, . - ap_s_apstartup32


# To be filled in by the BSP
.global ap_kernel_cr3
.type   ap_kernel_cr3, @object
ap_kernel_cr3:
    .skip 4


.global ap_gdt32_ptr
.type   ap_gdt32_ptr, @object
ap_gdt32_ptr:
    .skip 2	# uint16 limit
    .skip 4	# uint32 addr


.global ap_gdt32
.type   ap_gdt32, @object
ap_gdt32:
    .skip 0x70


.global ap_idt_ptr
ap_idt_ptr:
	.word 2048	# uint16 limit
	.int ap_idt	# uint32 addr
.global ap_idt
.type   ap_idt, @object
ap_idt:
    .skip 2048


.align 0x1000
.global ap_startup_stack # Use space between gdt and pml4 for stack
.type   ap_startup_stack, @object
ap_startup_stack:

.global ap_paging_root # PD
.type   ap_paging_root, @object
ap_paging_root:
.skip 0x1000
ap_paging_root_end:
.global ap_paging_root_end


.code32
