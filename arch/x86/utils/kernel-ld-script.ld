/********************************************************************
*
*  $Id: kernel-ld-script.ld,v 1.6 2005/04/23 11:56:34 nomenquis Exp $
*
*  $Log: kernel-ld-script.ld,v $
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
LS_Virt = 0x80100000; /* 2 gig + 1 meg due to 4m pages for kernel area*/
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

}

/* bug in MinGW? I get a bad executable file unless these
sections are here... */
    .stab :
    {
	*(.stab)
    }
    .stabstr :
    {
	*(.stabstr)
    }
/* symbols to mark end of kernel */
		
}
kernel_end_address = .;
