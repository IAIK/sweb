asm(".code32");
asm(".equ BASE,0xFFFFFFFF80000000");
asm(".equ PHYS_BASE,0xFFFFFFFF00000000");
#include "types.h"
#include "offsets.h"
#include "multiboot.h"
#include "segment-definitions.h"

#if A_BOOT == A_BOOT | OUTPUT_ENABLED
#define PRINT(X) print(TRUNCATE(X))
#else
#define PRINT(X)
#endif

#define TRUNCATE(X) (char*)(((unsigned int)(((char*)X)+0x7FFFFFFF))+1) // virtual to physical address

#define MULTIBOOT_PAGE_ALIGN (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_WANT_VESA (1<<2)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_WANT_VESA)
#define MULTIBOOT_CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

extern uint32 bss_start_address;
extern uint32 bss_end_address;
extern uint8 boot_stack[];
extern PageMapLevel4Entry kernel_page_map_level_4[];

SegmentDescriptor gdt[7];

struct GDT32Ptr
{
    uint16 limit;
    uint32 addr;
}__attribute__((__packed__));

typedef struct
{
    uint32 reserved_0;
    uint32 rsp0_l;
    uint32 rsp0_h;
    uint32 rsp1_l;
    uint32 rsp1_h;
    uint32 rsp2_l;
    uint32 rsp2_h;
    uint32 reserved_1;
    uint32 reserved_2;
    uint32 ist0_l;
    uint32 ist0_h;
    uint32 reserved_3[15];
}__attribute__((__packed__)) TSS;

TSS g_tss;

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

void print(const char* p)
{
  while (*p)
    asm volatile ("outb %b0, %w1" : : "a"(*p++), "d"(0xe9));
}

static void memset(char* block, char c, size_t length)
{
  for (size_t i = 0; i < length; ++i)
    block[i] = c;
}

extern uint8 boot_stack[];


static void setSegmentDescriptor(SegmentDescriptor* desc, uint32 baseH, uint32 baseL, uint32 limit, uint16 type)
{
  if((baseH != 0) && (type & 0x10))
  {
    PRINT("WARNING: Only TSS and LDT segment descriptors have a 64-bit base address\n");
  }
  if((type & D_64BIT) && (type & D_SIZE))
  {
    PRINT("WARNING: 64 bit and size flags must not be used at the same time in segment descriptors\n");
  }

  desc->sysseg.baseLL = (uint16) (baseL & 0xFFFF);
  desc->sysseg.baseLM = (uint8) ((baseL >> 16U) & 0xFF);
  desc->sysseg.baseLH = (uint8) ((baseL >> 24U) & 0xFF);
  desc->sysseg.baseH = baseH;
  desc->sysseg.limitL = (uint16) (limit & 0xFFFF);
  desc->sysseg.limitH = (uint8) (((limit >> 16U) & 0xF));

  // Bytes 5 + 6 contain segment type
  *((uint16*)(((uint8*)desc) + 5)) |= type;
}

extern "C" void entry()
{
  asm("mov %ebx,multi_boot_structure_pointer - BASE");
  PRINT("Booting...\n");
  PRINT("Clearing Framebuffer...\n");
  memset((char*) 0xB8000, 0, 80 * 25 * 2);

  PRINT("Clearing BSS...\n");
  char* bss_start = TRUNCATE(&bss_start_address);
  memset(bss_start, 0, TRUNCATE(&bss_end_address) - bss_start);

  PRINT("Initializing Kernel Paging Structures...\n");
  asm("movl $kernel_page_directory_pointer_table - BASE + 3, kernel_page_map_level_4 - BASE\n"
      "movl $0, kernel_page_map_level_4 - BASE + 4\n");
  asm("movl $kernel_page_directory - BASE + 3, kernel_page_directory_pointer_table - BASE\n"
      "movl $0, kernel_page_directory_pointer_table - BASE + 4\n");
  asm("movl $0x83, kernel_page_directory - BASE\n"
      "movl $0, kernel_page_directory - BASE + 4\n");

  PRINT("Enable PSE and PAE...\n");
  asm("mov %cr4,%eax\n"
      "or $0x20, %eax\n"
      "mov %eax,%cr4\n");

  PRINT("Setting CR3 Register...\n");
  asm("mov %[pd],%%cr3" : : [pd]"r"(TRUNCATE(kernel_page_map_level_4)));

  PRINT("Enable EFER.LME and EFER.NXE...\n");
  asm("mov $0xC0000080,%ecx\n"
      "rdmsr\n"
      "or $0x900,%eax\n"
      "wrmsr\n");

  asm("push $2\n"
      "popf\n");

  PRINT("Enable Paging...\n");
  asm("mov %cr0,%eax\n"
      "or $0x80000001,%eax\n"
      "mov %eax,%cr0\n");

  PRINT("Setup TSS...\n");
  TSS* g_tss_p = (TSS*) TRUNCATE(&g_tss);
  g_tss_p->ist0_h = -1U;
  g_tss_p->ist0_l = (uint32) TRUNCATE(boot_stack) | 0x80004000;
  g_tss_p->rsp0_h = -1U;
  g_tss_p->rsp0_l = (uint32) TRUNCATE(boot_stack) | 0x80004000;

  PRINT("Setup Segments...\n");

  SegmentDescriptor* gdt_p = (SegmentDescriptor*) TRUNCATE(&gdt);

  setSegmentDescriptor(gdt_p + KERNEL_CS_INDEX, 0, 0, 0, D_T_CODE | D_DPL0 | D_PRESENT | D_G_PAGE | D_64BIT | D_READABLE);
  setSegmentDescriptor(gdt_p + KERNEL_DS_INDEX, 0, 0, 0, D_T_DATA | D_DPL0 | D_PRESENT | D_G_PAGE | D_SIZE  | D_WRITEABLE);
  setSegmentDescriptor(gdt_p + USER_CS_INDEX,   0, 0, 0, D_T_CODE | D_DPL3 | D_PRESENT | D_G_PAGE | D_64BIT | D_READABLE);
  setSegmentDescriptor(gdt_p + USER_DS_INDEX,   0, 0, 0, D_T_DATA | D_DPL3 | D_PRESENT | D_G_PAGE | D_SIZE  | D_WRITEABLE);
  setSegmentDescriptor(gdt_p + KERNEL_TSS_INDEX, -1U, (uint32) TRUNCATE(&g_tss) | 0x80000000, sizeof(TSS) - 1, D_T_TSS_AVAIL | D_DPL0 | D_PRESENT);

  PRINT("Loading Long Mode GDT...\n");

  struct GDT32Ptr gdt32_ptr;
  gdt32_ptr.limit = sizeof(gdt) - 1;
  gdt32_ptr.addr = (uint32) gdt_p;
  asm("lgdt %[gdt_ptr]" : : [gdt_ptr]"m"(gdt32_ptr));
  asm("mov %%ax, %%ds\n" : : "a"(KERNEL_DS));

  PRINT("Setting Long Mode Segment Selectors...\n");
  asm("mov %%ax, %%ds\n"
      "mov %%ax, %%es\n"
      "mov %%ax, %%ss\n"
      "mov %%ax, %%fs\n"
      "mov %%ax, %%gs\n"
      : : "a"(KERNEL_DS));

  PRINT("Calling entry64()...\n");
  asm("ljmp %[cs],$entry64-BASE\n" : : [cs]"i"(KERNEL_CS));

  PRINT("Returned from entry64()? This should never happen.\n");
  asm("hlt");
}
asm(".code64");
