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
  LS_START = .;

  .text ALIGN(4096) : AT(LS_Phys + (LS_Code - LS_START))
  {
    PROVIDE(kernel_start_address = ABSOLUTE(.));

    LS_Code = .;

    text_start_address = .;
    *(.mboot)
    . = ALIGN(4096);
    apstartup_text_begin = .;
    *(.text.apstartup*)
    apstartup_text_end = .;
    . = ALIGN(4096);
    *(.text)
    *(.text.*)
    text_end_address = .;
    *(.gnu.linkonce.r.*)
    ro_data_start_address = .;
    *(.rodata*)
    ro_data_end_address = .;
  }

  .tdata ALIGN(4096) : AT(LS_Phys + (LS_tdata - LS_START))
  {
    LS_tdata = .;
    cls_start = .;
    tdata_start = .;
    *(.tdata)
    tdata_end = .;
  }

  .tbss ALIGN(4096) : AT(LS_Phys + (LS_tbss - LS_START))
  {
    LS_tbss = .;
    tbss_start = .;
    *(.tbss)
    . = ALIGN(8) + 8; /* For C++11 tread_local init-on-first-use flag */
    tbss_end = .;
    cls_end = .;
    . = ALIGN(8) + 8;
  }

  .data ALIGN(4096) : AT(LS_Phys + (LS_Data - LS_START))
  {
    LS_Data = .;
    data_start_address = .;
    *(.data)
    . = ALIGN(4096);
    *(.gdt_stuff)
    data_end_address = .;
  }

  .bss ALIGN(4096) : AT(LS_Phys + (LS_Bss - LS_START))
  {
    LS_Bss = .;
    bss_start_address = .;
    *(.bss)
    *(COMMON) /* common symbols, usually placed in .bss */
    . = ALIGN(4096);
    bss_end_address = .;
    PROVIDE(kernel_end_address = .);
  }
}
