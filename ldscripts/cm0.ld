ENTRY(start)

SECTIONS
{
    intbl    0x00000000: { *(Exceptions) }
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
    eob : { *(EoB) }
}