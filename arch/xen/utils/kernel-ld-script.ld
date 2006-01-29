OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
LS_Virt = 0x80000000;
SECTIONS
{
kernel_start_address = .;
 _text = .;
 .text LS_Virt : 
 {
  LS_Code = .; /*from orig*/
  text_start_address = .;
  *(.mboot)
	*(.text)
  text_end_address = .; /*from orig*/
  *(.gnu.linkonce.r.*)  /*from orig*/
	*(.gnu.warning) /*orig doesn't have this*/


  _etext = .;			/* End of text section */

  ro_data_start_address = .; /*from orig*/
  *(.rodata) *(.rodata.*) 
  ro_data_end_address = .; /*from orig*/
  . = ALIGN(4096); /*from orig*/

  /*from orig start*/
    stab_start_address_nr = .;
    [ .stab ]
    stab_end_address_nr = .; 
    
    stabstr_start_address_nr = .;
    [ .stabstr ]
    stabstr_end_address_nr = .;    
    . = ALIGN(4096);
  /*from orig end*/
	} = 0x9090 /*orig doesn't have this*/
  .data : {			/* Data */
    LS_Data = .; /*from orig*/
    data_start_address = .; /*from orig*/
    *(.data) 
    . = ALIGN(4096);       /*from orig*/
    data_end_address = .;  /*from orig*/
    . = ALIGN(4096);

	  CONSTRUCTORS /*needed?*/
	}

  _edata = .;			/* End of data section */

  . = ALIGN(8192);		/* init_task */
  .data.init_task : { *(.data.init_task) }

  . = ALIGN(4096);
  .data.page_aligned : { *(.data.idt) }

  . = ALIGN(32);
  .data.cacheline_aligned : { *(.data.cacheline_aligned) }

  __bss_start = .;		/* BSS */
  .bss : {
  bss_start_address = .; /*from orig*/
	*(.bss)
  /*from orig start*/
    *(COMMON) /* "common" variables, I have no idea where this comes from*/ 
    . = ALIGN(4096);
    *(.paging_stuff)  /*from orig WARNING: Most likely to change because of xen!*/
    . = ALIGN(4096);
    bss_end_address = .;
  /*from orig end*/
	}
  _end = . ;

  /* Sections to be discarded */
  /DISCARD/ : {
	*(.text.exit)
	*(.data.exit)
	*(.exitcall.exit)
	}
}

/* symbols to mark end of kernel */		
kernel_end_address = .;
