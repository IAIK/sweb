asm(".code32");
asm(".equ BASE,0xFFFFFFFF80000000");
#include "types.h"
#include "offsets.h"
#include "multiboot.h"

#define MULTIBOOT_PAGE_ALIGN (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_WANT_VESA (1<<2)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA)
#define MULTIBOOT_CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

static const struct {
  uint32 magic = MULTIBOOT_HEADER_MAGIC;
  uint32 flags = MULTIBOOT_HEADER_FLAGS;
  uint32 checksum = MULTIBOOT_CHECKSUM;
  uint32 mode = 0;
  uint32 widht = 800;
  uint32 height = 600;
  uint32 depth = 32;
} mboot __attribute__ ((section (".mboot")));

extern "C" void _entry();

extern "C" void print(const char* c)
{
  asm("outb %b0, %w1" : : "a"(c), "d"(0xe9));
}

extern "C" void _entry()
{
  asm("mov %ebx,multi_boot_structure_pointer - BASE");

  asm("mov $0xB8000, %edi\n"
      "mov $0xB8FA0, %ecx\n"
      "sub %edi, %ecx\n"
      "xor %eax, %eax\n"
      "rep stosb\n");
  asm("mov $bss_start_address - BASE, %edi\n"
      "mov $bss_end_address - BASE, %ecx\n"
      "sub %edi, %ecx\n"
      "xor %eax, %eax\n"
      "rep stosb\n");

  asm("mov $boot_stack + 0x4000 - BASE, %esp\n"
      "mov %esp, %ebp\n");

  asm("call initialiseBootTimePaging\n");

  asm("mov %cr4,%eax\n"
      "bts $4, %eax\n"
      "bts $5, %eax\n"
      "mov %eax,%cr4\n");

  asm("mov $kernel_page_map_level_4 - BASE,%eax\n"
      "mov %eax,%cr3\n");

  asm("mov $0xC0000080,%ecx\n"
      "rdmsr\n"
      "or $0x100,%eax\n"
      "rdmsr\n");

  asm("push $2\n"
      "popf\n");

  asm("mov %cr0,%eax\n"
      "or $0x80000001,%eax\n"
      "mov %eax,%cr0\n");

  /*
  mov eax, g_tss - PHYS_BASE
  mov word[tss.base_low - BASE], ax
  shr eax, 16
  mov byte[tss.base_high_word_low - BASE], al
  shr eax, 8
  mov byte[tss.base_high_word_high - BASE], al

  lgdt [gdt_ptr - BASE]

  EXTERN entry64
  jmp LINEAR_CODE_SEL:(entry64-BASE)

  hlt

  ret
*/
  asm("hlt");
  _entry();
}

extern "C" void initialiseBootTimePaging()
{
  asm("movl $kernel_page_directory_pointer_table - BASE + 3, kernel_page_map_level_4 - BASE\n"
      "movl $0, kernel_page_map_level_4 - BASE + 4\n");
  asm("movl $kernel_page_directory - BASE + 3, kernel_page_directory_pointer_table - BASE\n"
      "movl $0, kernel_page_directory_pointer_table - BASE + 4\n");
  asm("movl $0x83, kernel_page_directory - BASE\n"
      "movl $0, kernel_page_directory - BASE + 4\n");
}

asm(".code64");
