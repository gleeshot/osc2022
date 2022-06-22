__heap_size  = 0x00200000;

SECTIONS
{
    . = 0x80000;
    __start = .;
    .text :
    {
        KEEP(*(.text.boot))
    }
    .rodata :
    {
        *(.rodata)
    }
    .bss (NOLOAD) :
    {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    __heap_start = .;
    . += __heap_size;
    __heap_end = .;
    _end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;