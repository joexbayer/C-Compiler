add:
pushl %ebp
movl %esp, %ebp
subl $4, %esp
leal -4(%ebp), %eax
pushl %eax
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
addl %ebx, %eax
pushl %eax
movl $10, %eax
popl %ebx
addl %ebx, %eax
popl %ebx
movl %eax, (%ebx)
movl -4(%ebp), %eax
addl $4, %esp
popl %ebp
ret

perform:
pushl %ebp
movl %esp, %ebp
subl $4, %esp
leal -4(%ebp), %eax
pushl %eax
movl $1, %eax
pushl %eax
movl $2, %eax
pushl %eax
call add
addl $8, %esp
popl %ebx
movl %eax, (%ebx)
movl -4(%ebp), %eax
addl $4, %esp
popl %ebp
ret

main:
pushl %ebp
movl %esp, %ebp
subl $4, %esp
leal -4(%ebp), %eax
pushl %eax
call perform
popl %ebx
movl %eax, (%ebx)
movl -4(%ebp), %eax
addl $4, %esp
popl %ebp
ret

.globl _start
_start:

    # Call main
    call main
    movl %eax, %ebx

    # Exit program
    movl $1, %eax       # syscall number (sys_exit)
    int $0x80           # call kernel