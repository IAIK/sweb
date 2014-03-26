ENTRY (entry)

SECTIONS
{
    . = 0;
    .text : { *(.text*) *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON*) }
}
