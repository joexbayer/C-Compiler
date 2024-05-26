add:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
subl $4, %esp
# Assignment
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
# Cleaning up stack frame
addl $4, %esp
popl %ebp
ret

perform:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
subl $16, %esp
# Assignment
movl $20, -4(%ebp)
# Assignment
movl $6, -16(%ebp)
# Assignment
movl $10, -12(%ebp)
# Assignment
leal -16(%ebp), %eax
addl $8, %eax
pushl %eax
# Function call
leal -16(%ebp), %eax
movl 0(%eax), %eax
pushl %eax
leal -16(%ebp), %eax
movl 4(%eax), %eax
pushl %eax
call add
addl $8, %esp # Cleanup stack pushed arguments
popl %ebx
movl %eax, (%ebx)
leal -16(%ebp), %eax
movl 8(%eax), %eax
# Cleaning up stack frame
addl $16, %esp
popl %ebp
ret

main:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
subl $4, %esp
# Assignment
leal -4(%ebp), %eax
pushl %eax
# Function call
call perform
popl %ebx
movl %eax, (%ebx)
movl -4(%ebp), %eax
# Cleaning up stack frame
addl $4, %esp
popl %ebp
ret


.globl _start
_start:
  call main
  movl %eax, %ebx
  movl $1, %eax
  int $0x80
