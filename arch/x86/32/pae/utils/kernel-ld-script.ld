/**
 * this linker script tells ld how to link and which symbols to add to the
 * kernel.x binary
 */

/* let the linker use its 'native' format  (ELF/COFF/PE) */
OUTPUT_FORMAT("elf32-i386")
ENTRY(entry)
LS_Phys = 0x100000; /* 1 meg */
LS_Virt = 0x80000000; /* 2 gig + 1 meg due to 4m pages for kernel area*/

/* AP_STARTUP = 0x0000; */

SECTIONS
{
  .mboot ALIGN(4096): AT(LS_Phys)
  {
    KEEP(*(.mboot))
  }

  . = 0;

  /* . = AP_STARTUP; */
  .text.apstartup ALIGN(4096) : AT(LS_Phys + SIZEOF(.mboot))
  {
    apstartup_text_begin = .;
    *(.text.apstartup*)
    apstartup_text_end = .;
  }

  . = LS_Virt;
  LS_START = .;

  PROVIDE(kernel_start_address = ABSOLUTE(.));

  mboot_load_begin = .;
  . += SIZEOF(.mboot);
  mboot_load_end = .;

  /* .mboot.load ALIGN(4096) (INFO) : AT(0x0) */
  /* { */
  /*   . += SIZEOF(.mboot); */
  /* } */

  apstartup_text_load_begin = .;
  . += SIZEOF(.text.apstartup);
  apstartup_text_load_end = .;

  .text ALIGN(4096) : AT(LS_Phys + (LS_Code - LS_START))
  {
    LS_Code = .;

    text_start_address = .;
    /*   KEEP(*(.mboot)) */
    /* . = ALIGN(4096); */
    /* apstartup_text_begin = .; */
    /* *(.text.apstartup*) */
    /* apstartup_text_end = .; */
    /* . = ALIGN(4096); */
    *(.text)
    *(.text.*)
    text_end_address = .;
    *(.gnu.linkonce.r.*)
    ro_data_start_address = .;
    *(.rodata*)
    ro_data_end_address = .;
    *(.eh_frame)
    *(.eh_frame_hdr)
    . = ALIGN(4096);
  }

  /* .text.apstartup.load ALIGN(4096) : AT(LS_Phys + (LS_apstartup_load - LS_START)) */
  /* { */
  /*   LS_apstartup_load = .; */
  /*   apstartup_text_load_begin = .; */
  /*   . += SIZEOF(.text.apstartup); */
  /*   apstartup_text_load_end = .; */
  /*   . = ALIGN(4096); */
  /* } */

  /* .eh_frame ALIGN(4096) : AT(LS_Phys + (LS_eh_frame - LS_START)) */
  /* { */
  /*   LS_eh_frame = .; */
  /*   *(.eh_frame) */
  /*   *(.eh_frame_hdr) */
  /* } */

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

  .init_array : AT(LS_Phys + (__init_array_start - LS_START))
  {
    __init_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    __init_array_end = .;
  }

  .preinit_array : AT(LS_Phys + (__preinit_array_start - LS_START))
  {
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;
  }

  .fini_array : AT(LS_Phys + (__fini_array_start - LS_START))
  {
    __fini_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    __fini_array_end = .;
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
