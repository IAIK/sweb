/********************************************************************
*
*  $Id: kernel-ld-script.ld,v 1.2 2005/04/19 17:51:49 nomenquis Exp $
*
*  $Log: kernel-ld-script.ld,v $
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

    .text LS_Virt : AT(LS_Phys)
    {
	kernel_start_address = .;
	text_start_address = .;
	LS_Code = .;

/* symbols to mark start of code segment */
	code = .; _code = .;
/* kernel code */
	*(.text)
	text_end_address = .;
	*(.gnu.linkonce.r.*)
/* .rodata is the ELF constant data section */
	*(.rodata*)
	. = ALIGN(4096);
    }
/* data section */
    .data  ALIGN(4096) : AT(LS_Phys + (LS_Data - LS_Code))
    {
	LS_Data = .;
/* symbols to mark start of data segment */
	data = .; _data = .;
/* kernel data */
	*(.data)
  . = ALIGN(4096);
    }
    .bss : AT(LS_Phys + (LS_Bss - LS_Code))
    {
	LS_Bss = .;
/* symbols to mark start of BSS segment */
	bss = .; _bss = .;
/* kernel BSS */
	*(.bss)
	*(COMMON) /* "common" variables */
	. = ALIGN(4096);
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
    end = .; _end = .; 
		kernel_end_address = .;
		
}
