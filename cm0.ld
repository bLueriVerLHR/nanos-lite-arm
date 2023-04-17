ENTRY(start)

SECTIONS
{
    /* .boot    0x00000000: { *(Boot) } */
    ldmsp    0x00000000: { *(Exceptions) }
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