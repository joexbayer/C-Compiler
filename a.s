add:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
movl 8(%ebp), %eax
popl %ebx
movl %eax, (%ebx)
movl $0, %eax
# Cleaning up stack frame
popl %ebp
ret

main:
# Setting up stack frame
pushl %ebp
movl %esp, %ebp
subl $8, %esp
movl $1, -4(%ebp)
movl -8(%ebp), %eax
pushl %eax
movl $0, (%eax)
# Function call
movl -8(%ebp), %eax
pushl %eax
movl $2, %eax
pushl %eax
call add
addl $8, %esp # Cleanup stack pushed arguments
movl -4(%ebp), %eax
# Cleaning up stack frame
addl $8, %esp
popl %ebp
ret


.globl _start
_start:
  call main
  movl %eax, %ebx
  movl $1, %eax
  int $0x80
