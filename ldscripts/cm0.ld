ENTRY(start)

SECTIONS
{
    .interp 0x00000000: { *(.interp) }
    .text : {
        *(.text)
        *(.text*)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
    .eob : { *(.eob) }
}