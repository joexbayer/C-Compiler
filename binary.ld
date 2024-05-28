ENTRY(_start);

SECTIONS
{
    . = 0x08048000;
    .text : {
        *(.data)
    }
    /DISCARD/ : { *(.note.GNU-stack) }
}
