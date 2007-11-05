/********************************************************************
*
*  $Id: kernel-ld-script.ld,v 1.11 2005/07/22 15:29:42 nomenquis Exp $
*
*  $Log: kernel-ld-script.ld,v $
*  Revision 1.10  2005/05/10 16:29:42  nomenquis
*  stabs back into ro data
*
*  Revision 1.9  2005/05/08 21:43:55  nelles
*  changed gcc flags from -g to -g3 -gstabs in order to
*  generate stabs output in object files
*  changed linker script to load stabs in kernel
*  in bss area so GRUB loads them automaticaly with
*  the bss section
*
*  changed StupidThreads in main for testing purposes
*
*  Revision 1.8  2005/05/02 18:33:05  nelles
*  added stab_start, stab_end and stabstr start/end symbols to the ld script
*
*  Revision 1.7  2005/04/26 10:23:54  nomenquis
*  kernel at 2gig again, not 2gig + 1m since were not using 4m pages anymore
*
*  Revision 1.6  2005/04/23 11:56:34  nomenquis
*  added interface for memory maps, it works now
*
*  Revision 1.5  2005/04/22 17:21:39  nomenquis
*  added TONS of stuff, changed ZILLIONS of things
*
*  Revision 1.4  2005/04/20 09:00:29  nomenquis
*  removed lots of trash symbols, and added "nice" symbol names
*  also added docs about these symbols into the wiki
*
*  Revision 1.3  2005/04/20 07:09:59  nomenquis
*  added inital paging for the kernel, plus mapping to two gigs
*  hoever, the kernel now is at 1meg phys and 2gig + 1 meg virtual due to these 4m pages
*
*  Revision 1.2  2005/04/19 17:51:49  nomenquis
*  added some symbols
*
*  Revision 1.1  2005/04/12 18:42:51  nomenquis
*  changed a zillion of iles
*
*
********************************************************************/

/* let the linker use its 'native' format  (ELF/COFF/PE) */
OUTPUT_FORMAT("elf32-i386")
/* no leading underscore for symbols handled in asm: */
ENTRY(entry)
LS_Phys = 0x100000; /* 1 meg */
/*LS_Virt = 0x100000; /* 1 meg */
/* use this line instead when paging works properly! */
LS_Virt = 0x80000000; /* 2 gig */
SECTIONS
{
kernel_start_address = .;
  .text LS_Virt : AT(LS_Phys)
  {
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
    
    stab_start_address_nr = .;
    [ .stab ]
    stab_end_address_nr = .; 
    
    stabstr_start_address_nr = .;
    [ .stabstr ]
    stabstr_end_address_nr = .;    
    . = ALIGN(4096);
  }


  .data  ALIGN(4096) : AT(LS_Phys + (LS_Data - LS_Code))
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
  
  .bss : AT(LS_Phys + (LS_Bss - LS_Code))
  {
    LS_Bss = .;
    bss_start_address = .;
    *(.bss)
    *(COMMON) /* "common" variables, I have no idea where this comes from*/ 
    . = ALIGN(4096);
    *(.paging_stuff)
    . = ALIGN(4096);
    bss_end_address = .;
    /* lets try this bad voodo shit 
       load stabs in BSS to avoid relocation 
       and to force GRUB to load it */

  }

/* bug in MinGW? I get a bad executable file unless these
sections are here... */
/*
  .stab : AT(LS_Phys + (LS_Stab - LS_Code))
  {
    LS_Stab = .;
    stab_start_address = .;
    *(.stab)
    stab_end_address = .;
  }
    
  .stabstr : AT(LS_Phys + (LS_Stabstr - LS_Code))
  {
    LS_Stabstr = .;
    stabstr_start_address = .;
    *(.stabstr)
    stabstr_end_address = .;
  }
  */
/* symbols to mark end of kernel */
	
/*
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
*/ /*
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }

*/
/*
  .debug : AT(LS_Phys + (LS_Debug - LS_Code))
  {
    LS_Debug = .;
    debug_start_address = .;
    *(.debug)
    debug_end_address = .;
  }

  .line : AT(LS_Phys + (LS_Line - LS_Code))
  {
    LS_Line = .;
    line_start_address = .;
    *(.line)
    line_end_address = .;
  }

  .debug_srcinfo : AT(LS_Phys + (LS_DebugSrcInfo - LS_Code))
  {
    LS_DebugSrcInfo = .;
    debug_srcinfo_start_address = .;
    *(.debug_srcinfo)
    debug_srcinfo_end_address = .;
  }

  .debug_sfnames : AT(LS_Phys + (LS_DebugSfNames - LS_Code))
  {
    LS_DebugSfNames = .;
    line_start_address = .;
    *(.debug_sfnames)
    line_end_address = .;
  }
*/
/*
  .stab :  AT(LS_Phys + (LS_1 - LS_Code)) {LS_1 = .; *(.stab) }
  .stabstr :  AT(LS_Phys + (LS_2 - LS_Code)) {LS_2 = .;  *(.stabstr) }
  .stab.excl :  AT(LS_Phys + (LS_3 - LS_Code)) {LS_3 = .;  *(.stab.excl) }
  .stab.exclstr :  AT(LS_Phys + (LS_4 - LS_Code)) {LS_4 = .;  *(.stab.exclstr) }
  .stab.index :  AT(LS_Phys + (LS_5 - LS_Code)) {LS_5 = .;  *(.stab.index) }
  .stab.indexstr :  AT(LS_Phys + (LS_6 - LS_Code)) {LS_6 = .;  *(.stab.indexstr) }
  .comment :  AT(LS_Phys + (LS_7 - LS_Code)) {LS_7 = .;  *(.comment) }

*/
}
kernel_end_address = .;
