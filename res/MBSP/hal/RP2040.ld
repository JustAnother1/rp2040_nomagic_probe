ENTRY(Reset_Handler)
TARGET(binary)
INPUT("hal/2ndstage.bin")
OUTPUT_FORMAT(default)

MEMORY
{
    FLASH (xr) :ORIGIN = 0x10000000, LENGTH = 2048K
    RAM (xwr) :ORIGIN = 0x20000000, LENGTH = 264K
}

SECTIONS
{
.text 0x10000000 : {
        "hal/2ndstage.bin"
        KEEP (*(.vectors))
    } > FLASH
.text : {
    *(.text)
} > FLASH
.data : {
    . = ALIGN(4);
    __data_start = .;

    *(.data)
    *(.data.*)

    . = ALIGN(4);
    __data_end = .;
} >RAM AT> FLASH

/* used by the startup to initialize data */
__data_in_flash = LOADADDR(.data);
.bss (NOLOAD) : ALIGN(4) {
    __bss_start = .;
    *(.bss)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
    _end = __bss_end;
    end = _end;
}> RAM
}
