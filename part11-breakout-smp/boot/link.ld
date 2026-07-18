SECTIONS
{
    . = 0x80000;     /* Kernel load address for AArch64 */
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    .cpu1Stack :
    {
        . = ALIGN(16);
        __stack_start  = .;
        . = . + 0x4000;      /* 16KB - the gfx core keeps a paddle-sized bitmap on its stack */
        __cpu1_stack  = .;
    }
    .cpu2Stack :
    {
        . = . + 0x4000;
        __cpu2_stack  = .;
    }
    .cpu3Stack :
    {
        . = . + 0x4000;
        __cpu3_stack  = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;
