/**
 * this linker script tells ld how to link and which symbols to add to the
 * kernel.x binary
 */
 
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(entry)
LS_Phys = 0x100000;
LS_Virt = 0xFFFFFFFF80000000;
UpperToLower = 0xFFFFFFFF00000000;
SECTIONS
{
  . = LS_Virt + LS_Phys;

  .text : AT(ADDR(.text) - LS_Virt)
  {
    PROVIDE(kernel_start_address = ABSOLUTE(.));

    LS_Code = .;

    text_start_address = .;
    *(.mboot)
    *(.text)
    *(.text.*)
    text_end_address = .;
    *(.gnu.linkonce.r.*)
    ro_data_start_address = .;
    *(.rodata*)
    ro_data_end_address = .;
    . = ALIGN(4096);
  }

  .data : AT(ADDR(.data) - LS_Virt)
  {
    LS_Data = .;
    data_start_address = .;
    *(.data)
    . = ALIGN(4096);
    *(.gdt_stuff)
    data_end_address = .;
    . = ALIGN(4096);
  }

  .bss : AT(ADDR(.bss) - LS_Virt)
  {
    LS_Bss = .;
    bss_start_address = .;
    . = ALIGN(4096);
    *(.bss)
    . = ALIGN(4096);
    *(COMMON) /* common symbols, usually placed in .bss */
    . = ALIGN(4096);
    bss_end_address = .;
  }
  
  PROVIDE(kernel_end_address = .);
}
