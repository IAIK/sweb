/**
 * this linker script tells ld how to link and which symbols to add to the
 * kernel.x binary
 */
 
/* let the linker use its 'native' format  (ELF/COFF/PE) */
OUTPUT_FORMAT("elf32-i386")
ENTRY(entry)
LS_Phys = 0x100000; /* 1 meg */
LS_Virt = 0x80000000; /* 2 gig + 1 meg due to 4m pages for kernel area*/
SECTIONS
{
  . = LS_Virt;
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
    . = ALIGN(4096);
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
    . = ALIGN(4) + 4; /* For C++11 tread_local init-on-first-use flag */
    tbss_end = .;
    cls_end = .;
    . = ALIGN(4) + 4;
  }

  .data  ALIGN(4096) : AT(LS_Phys + (LS_Data - LS_START))
  {
    LS_Data = .;
    data_start_address = .;
    *(.data)
    . = ALIGN(4096);
    *(.gdt_stuff)
    . = ALIGN(4096);
    *(COMMON) /* common symbols, usually placed in .bss */
    data_end_address = .;
    . = ALIGN(4096);
  }

  .bss ALIGN(4096) : AT(LS_Phys + (LS_Bss - LS_START))
  {
    LS_Bss = .;
    bss_start_address = .;
    *(.bss)
    . = ALIGN(4096);
    bss_end_address = .;
  }

   .stab : AT(LS_Phys + (LS_Stab - LS_START))
  {
    LS_Stab = .;
    stab_start_address_nr = .;
    *(.stab)
    stab_end_address_nr = .;
    . = ALIGN(4096);
  }
  

  .stabstr : AT(LS_Phys + (LS_Stabstr - LS_START))
  {
    LS_Stabstr = .;
    stabstr_start_address_nr = .;
    *(.stabstr)
    stabstr_end_address_nr = .;
    . = ALIGN(4096);

    PROVIDE(kernel_end_address = .);
  }

}
