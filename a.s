add:
# Setting up stack frame 5
pushl %ebp
movl %esp, %ebp
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
addl %ebx, %eax
# Cleaning up stack frame
popl %ebp
ret

sub:
# Setting up stack frame 20
pushl %ebp
movl %esp, %ebp
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
subl %ebx, %eax
# Cleaning up stack frame
popl %ebp
ret

mul:
# Setting up stack frame 35
pushl %ebp
movl %esp, %ebp
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
imull %ebx, %eax
# Cleaning up stack frame
popl %ebp
ret

div:
# Setting up stack frame 51
pushl %ebp
movl %esp, %ebp
# If statement
movl 8(%ebp), %eax
pushl %eax
movl $0, %eax
popl %ebx
cmpl %ebx, %eax
setne %al
movzb %al, %eax
cmpl $0, %eax
je .Lfalse0
# If true
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
popl %ebx
movl $0, %edx
idivl %ebx
jmp .Lend1
.Lfalse0:
movl $0, %eax
.Lend1:
# Cleaning up stack frame
popl %ebp
ret

main:
# Setting up stack frame 83
pushl %ebp
movl %esp, %ebp
subl $4, %esp
# Function call
movl $10, %eax
pushl %eax
movl $10, %eax
pushl %eax
call add -104
addl $8, %esp # Cleanup stack pushed arguments
movl %eax, -4(%ebp)
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
