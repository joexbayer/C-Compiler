/*  Bootloader...
 *
 *   Inspiried by stage0.s in tetris-os by JDH
 *   His video: https://www.youtube.com/watch?v=FaILnmUYS_U&t=647s
 *   Github: https://github.com/jdah/tetris-os
 */

.code16
.org 0
.text

.global _start
_start:
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    movw $0x3000, %sp

  
    movw $20, %cx
    movw $disk_address_packet, %si
    movw $0x1000, segment
    movw $1, sector

read_loop:
    movb $0x42, %ah
    int $0x13

    /* Check if still in same segment. */
    addw $64, sector
    addw $0x8000, offset
    jnc reading_same_segment
    addw $0x1000, segment
    movw $0x0000, offset
reading_same_segment:
    loop read_loop

    call set_a20

    /* enable the PE flag */
    movl %cr0, %eax
    orl $0x1, %eax
    movl %eax, %cr0

    jmp setup_gdt
setup_gdt:
    cli
    lgdt gdtp

    /* Setup GDT descriptor for data, setting registers. */
    movw $(data_descriptor - gdt_start), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x3000, %esp
    
    ljmp $0x8, $enter32

/* Set a20 line from: http://www.brokenthorn.com/Resources/OSDev9.html*/
set_a20: 
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     set_a20

    movb    $0xd1,%al               # 0xd1 -> port 0x64
    outb    %al,$0x64                                                                                

set_a20.2:
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     set_a20.2

    movb    $0xdf,%al               # 0xdf -> port 0x60
    outb    %al,$0x60
    retw

.code32
enter32:
    movw $0x3000, %sp
    movl $0x10000, %eax
    jmpl *%eax

inf:
    jmp inf

/* DAP (Disk Address Packet) */
disk_address_packet:
    .byte 0x10
    .byte 0x00
    .word 0x0040
offset:
    .word 0x0000
segment:
    .word 0x0000 /* will be set to $0x1000 */
sector:
    .quad 0x00000000
.align 16
gdtp:
    .word gdt_end - gdt_start - 1
    .long gdt_start
.align 16
gdt_start:
gdt_null:
    .quad 0

code_descriptor:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x00
data_descriptor:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10010010
    .byte 0b11001111
    .byte 0x00
gdt_end:

/* BOOT SIGNATURE */
.fill 510-(.-_start), 1, 0
.word 0xAA55