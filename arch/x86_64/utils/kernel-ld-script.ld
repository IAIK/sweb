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
    . = ALIGN(4096);
    *(.idt_stuff)
    data_end_address = .;
    . = ALIGN(4096);
  }
  /*
  .debug          . - UpperToLower : {
    debug_start = . + UpperToLower;
    *(.debug)
    debug_end = . + UpperToLower;
  }
  .line           . : {
    line_start = . + UpperToLower;
    *(.line)
    line_end = . + UpperToLower;
  }
  .debug_srcinfo  . : {
    debug_srcinfo_start = . + UpperToLower;
    *(.debug_srcinfo)
    debug_srcinfo_end = . + UpperToLower;
  }
  .debug_sfnames  . : {
    debug_sfnames_start = . + UpperToLower;
    *(.debug_sfnames)
    debug_sfnames_end = . + UpperToLower;
  }
  .debug_aranges  . : {
    debug_aranges_start = . + UpperToLower;
    *(.debug_aranges)
    debug_aranges_end = . + UpperToLower;
  }
  .debug_pubnames . : {
    debug_pubnames_start = . + UpperToLower;
    *(.debug_pubnames)
    debug_pubnames_end = . + UpperToLower;
  }
  .debug_info     . : {
    debug_info_start = . + UpperToLower;
    *(.debug_info .gnu.linkonce.wi.*)
    debug_info_end = . + UpperToLower;
  }
  .debug_abbrev   . : {
    debug_abbrev_start = . + UpperToLower;
    *(.debug_abbrev)
    debug_abbrev_end = . + UpperToLower;
  }
  .debug_line     . : {
    debug_line_start = . + UpperToLower;
    *(.debug_line)
    debug_line_end = . + UpperToLower;
  }
  .debug_frame    . : {
    debug_frame_start = . + UpperToLower;
    *(.debug_frame)
    debug_frame_end = . + UpperToLower;
  }
  .debug_str      . : {
    debug_str_start = . + UpperToLower;
    *(.debug_str)
    debug_str_end = . + UpperToLower;
  }
  .debug_loc      . : {
    debug_loc_start = . + UpperToLower;
    *(.debug_loc)
    debug_loc_end = . + UpperToLower;
  }
  .debug_macinfo  . : {
    debug_macinfo_start = . + UpperToLower;
    *(.debug_macinfo)
    debug_macinfo_end = . + UpperToLower;
  }
  .debug_weaknames . : {
    debug_weaknames_start = . + UpperToLower;
    *(.debug_weaknames)
    debug_weaknames_end = . + UpperToLower;
  }
  .debug_funcnames . : {
    debug_funcnames_start = . + UpperToLower;
    *(.debug_funcnames)
    debug_funcnames_end = . + UpperToLower;
  }
  .debug_typenames . : {
    debug_typenames_start = . + UpperToLower;
    *(.debug_typenames)
    debug_typenames_end = . + UpperToLower;
  }
  .debug_varnames  . : {
    debug_varnames_start = . + UpperToLower;
    *(.debug_varnames)
    debug_varnames_end = . + UpperToLower;
  }
*/
  .bss : AT(ADDR(.bss) - LS_Virt)
  {
    LS_Bss = .;
    bss_start_address = .;
    . = ALIGN(4096);
    *(.bss)
    . = ALIGN(4096);
    *(COMMON) /* common symbols, usually placed in .bss */
    . = ALIGN(4096);
    *(.paging_stuff)
    . = ALIGN(4096);
    bss_end_address = .;
  }
  
  PROVIDE(kernel_end_address = .);
}
