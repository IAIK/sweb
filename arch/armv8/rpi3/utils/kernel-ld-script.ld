/**
 * this linker script tells ld how to link and which symbols to add to the
 * kernel.x binary
 */

ENTRY(entry)
LS_Virt = 0xFFFFFFC000000000;
LS_Phys = 0x0000000000000000;
/* Specify the memory areas */

SECTIONS
{
  .text LS_Virt : AT(LS_Phys)
  {
    PROVIDE(kernel_start_address = ABSOLUTE(.));

    LS_Code = .;

    text_start_address = .;
    *(.text.boot)
    *(.text)
    *(.text.*)
    text_end_address = .;
    ro_data_start_address = .;
    *(.rodata*)
    ro_data_end_address = .;
    swebdbg_start_address_nr  = .;
    *(.swebdbg)
    swebdbg_end_address_nr  = .;
  }

  .tdata ALIGN(4096) : AT(LS_Phys + (LS_tdata - LS_Code))
  {
    LS_tdata = .;
    cls_start = .;
    tdata_start = .;
    *(.tdata)
    tdata_end = .;
  }

  .tbss ALIGN(4096) : AT(LS_Phys + (LS_tbss - LS_Code))
  {
    LS_tbss = .;
    tbss_start = .;
    *(.tbss)
    . = ALIGN(8) + 8; /* For C++11 thread_local init-on-first-use flag */
    tbss_end = .;
    cls_end = .;
    . = ALIGN(8) + 8;
  }

  .data ALIGN(4096) : AT(LS_Phys + (LS_Data - LS_Code))
  {
    LS_Data = .;
    data_start_address = .;
    *(.data*)
    data_end_address = .;
  }

  .init_array : AT(LS_Phys + (__init_array_start - LS_Code))
  {
    __init_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    __init_array_end = .;
  }

  .preinit_array : AT(LS_Phys + (__preinit_array_start - LS_Code))
  {
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;
  }

  .fini_array : AT(LS_Phys + (__fini_array_start - LS_Code))
  {
    __fini_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    __fini_array_end = .;
  }

  .bss ALIGN(4096) : AT(LS_Phys + (LS_Bss - LS_Code))
  {
    LS_Bss = .;
    bss_start_address = .;
    *(.bss)
    . = ALIGN(4096);
    *(COMMON)
        . = ALIGN(4096);
    bss_end_address = .;

  }

  .stab : AT(LS_Phys + (LS_Stab - LS_Code))
  {
    LS_Stab = .;
    stab_start_address_nr = .;
    . = ALIGN(4096);
    *(.stab)
    . = ALIGN(4096);
    stab_end_address_nr = .;
  }

  .stabstr : AT(LS_Phys + (LS_Stabstr - LS_Code))
  {
    LS_Stabstr = .;
    stabstr_start_address_nr = .;
    . = ALIGN(4096);
    *(.stabstr)
    . = ALIGN(4096);
    stabstr_end_address_nr = .;

    PROVIDE(kernel_end_address = .);
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
  .comment 0 : { *(.comment) }
}
