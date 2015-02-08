asm(".code32");
asm(".equ BASE,0xFFFFFFFF80000000");
asm(".equ PHYS_BASE,0xFFFFFFFF00000000");
#include "types.h"
#include "offsets.h"
#include "multiboot.h"

#define MULTIBOOT_PAGE_ALIGN (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_WANT_VESA (1<<2)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA)
#define MULTIBOOT_CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

static const struct
{
    uint32 magic = MULTIBOOT_HEADER_MAGIC;
    uint32 flags = MULTIBOOT_HEADER_FLAGS;
    uint32 checksum = MULTIBOOT_CHECKSUM;
    uint32 mode = 0;
    uint32 widht = 800;
    uint32 height = 600;
    uint32 depth = 32;
} mboot __attribute__ ((section (".mboot")));

extern "C" void print(const char* p)
{
  while (*p) { asm volatile ("outb %b0, %w1" : : "a"(*p++), "d"(0xe9)); }
}

#define CALL32(X,Y) do { asm("push %[ptr]-BASE\n call %[f]" : : [ptr]"i"(Y), [f]"m"(X)); } while (0)

extern "C" void entry()
{
  asm("mov %ebx,multi_boot_structure_pointer - BASE");
  CALL32(print,"Booting...\n");
  CALL32(print,"Clearing Framebuffer...\n");
  asm("mov $0xB8000, %edi\n"
      "mov $0xB8FA0, %ecx\n"
      "sub %edi, %ecx\n"
      "xor %eax, %eax\n"
      "rep stosb\n");

  CALL32(print,"Clearing BSS...\n");
  asm("mov $bss_start_address - BASE, %edi\n"
      "mov $bss_end_address - BASE, %ecx\n"
      "sub %edi, %ecx\n"
      "xor %eax, %eax\n"
      "rep stosb\n");

  CALL32(print,"Switch to our own stack...\n");
  asm("mov $boot_stack + 0x4000 - BASE, %esp\n"
      "mov %esp, %ebp\n");

  CALL32(print,"Initializing Kernel Paging Structures...\n");
  asm("movl $kernel_page_directory_pointer_table - BASE + 3, kernel_page_map_level_4 - BASE\n"
      "movl $0, kernel_page_map_level_4 - BASE + 4\n");
  asm("movl $kernel_page_directory - BASE + 3, kernel_page_directory_pointer_table - BASE\n"
      "movl $0, kernel_page_directory_pointer_table - BASE + 4\n");
  asm("movl $0x83, kernel_page_directory - BASE\n"
      "movl $0, kernel_page_directory - BASE + 4\n");

  CALL32(print,"Enable PSE and PAE...\n");
  asm("mov %cr4,%eax\n"
      "bts $4, %eax\n"
      "bts $5, %eax\n"
      "mov %eax,%cr4\n");

  CALL32(print,"Setting CR3 Register...\n");
  asm("mov $kernel_page_map_level_4 - BASE,%eax\n"
      "mov %eax,%cr3\n");

  CALL32(print,"Enable EFER.LME...\n");
  asm("mov $0xC0000080,%ecx\n"
      "rdmsr\n"
      "or $0x100,%eax\n"
      "wrmsr\n");

  asm("push $2\n"
      "popf\n");

  CALL32(print,"Enable Paging...\n");
  asm("mov %cr0,%eax\n"
      "or $0x80000001,%eax\n"
      "mov %eax,%cr0\n");

  asm("mov $g_tss - PHYS_BASE, %eax\n"
      "mov %ax, tss.base_low - BASE\n"
      "shr $16, %eax\n"
      "mov %al, tss.base_high_word_low - BASE\n"
      "shr $8, %eax\n"
      "mov %al, tss.base_high_word_high - BASE\n");

  asm("lgdt gdt_ptr - BASE");
  asm("mov %%ax, %%ds\n"
      "mov %%ax, %%es\n"
      "mov %%ax, %%ss\n"
      "mov %%ax, %%fs\n"
      "mov %%ax, %%gs\n"
      : : "a"(KERNEL_DS));
  CALL32(print,"Calling startup()...\n");
  asm("ljmp %[cs],$entry64-BASE\n" : : [cs]"i"(KERNEL_CS));
  CALL32(print,"Returned from entry64()? This should never happen.\n");
  asm("hlt");
}
asm(".code64");
