add:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
movl %eax, 8(%ebx)
movl $0, %eax
# Cleaning up stack frame
popl %ebp
ret

main:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
subl $16, %esp
leal -16(%ebp), %eax
pushl %eax
# Assignment
# Reference
leal -12(%ebp), %eax
popl %ebx
movl %eax, (%ebx)
movl $1, -12(%ebp)
movl $123, -8(%ebp)
movl $0, -4(%ebp)
# Function call
movl -16(%ebp), %eax
pushl %eax
movl $5, %eax
pushl %eax
call add 5
addl $8, %esp # Cleanup stack pushed arguments
movl -16(%ebp), %eax
movl 8(%eax), %eax
# Cleaning up stack frame
addl $16, %esp
popl %ebp
ret

.globl _start
_start:
call main
movl %eax, %ebx
movl $1, %eax
int $0x80
