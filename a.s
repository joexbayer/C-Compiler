alloc:
# Setting up stack frame 5
pushl %ebp
movl %esp, %ebp
subl $4, %esp
# Function call
movl $128, %eax
pushl %eax
movl $192, %eax
pushl %eax
movl $0, %eax
pushl %eax
movl 8(%ebp), %eax
pushl %eax
movl $3, %eax
pushl %eax
movl $34, %eax
pushl %eax
popl %esi
popl %edx
popl %ecx
popl %ebx
popl %eax
popl %edi
pushl %ebp
xorl %ebp, %ebp
xorl %edi, %edi
dec %edi
int $0128
popl %ebp
movl %eax, -4(%ebp)
movl -4(%ebp), %eax
# Cleaning up stack frame
addl $4, %esp
popl %ebp
ret

free:
# Setting up stack frame 77
pushl %ebp
movl %esp, %ebp
# Function call
movl $128, %eax
pushl %eax
movl $91, %eax
pushl %eax
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
pushl %eax
movl $0, %eax
pushl %eax
movl $0, %eax
pushl %eax
popl %esi
popl %edx
popl %ecx
popl %ebx
popl %eax
popl %edi
pushl %ebp
xorl %ebp, %ebp
xorl %edi, %edi
dec %edi
int $0128
popl %ebp
movl $0, %eax
# Cleaning up stack frame
popl %ebp
ret

print:
# Setting up stack frame 134
pushl %ebp
movl %esp, %ebp
# Function call
movl $128, %eax
pushl %eax
movl $4, %eax
pushl %eax
movl $1, %eax
pushl %eax
movl 12(%ebp), %eax
pushl %eax
movl 8(%ebp), %eax
pushl %eax
movl $0, %eax
pushl %eax
popl %esi
popl %edx
popl %ecx
popl %ebx
popl %eax
popl %edi
pushl %ebp
xorl %ebp, %ebp
xorl %edi, %edi
dec %edi
int $0128
popl %ebp
movl $0, %eax
# Cleaning up stack frame
popl %ebp
ret

main:
# Setting up stack frame 191
pushl %ebp
movl %esp, %ebp
subl $4, %esp
# Function call
movl $4096, %eax
pushl %eax
call alloc -206
addl $4, %esp # Cleanup stack pushed arguments
movl %eax, -4(%ebp)
movl -4(%ebp), %eax
pushl %eax
movl $0, %eax
popl %ebx
addl %ebx, %eax
movl $72, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $1, %eax
popl %ebx
addl %ebx, %eax
movl $101, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $2, %eax
popl %ebx
addl %ebx, %eax
movl $108, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $3, %eax
popl %ebx
addl %ebx, %eax
movl $108, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $4, %eax
popl %ebx
addl %ebx, %eax
movl $111, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $5, %eax
popl %ebx
addl %ebx, %eax
movl $10, (%eax)
movl -4(%ebp), %eax
pushl %eax
movl $6, %eax
popl %ebx
addl %ebx, %eax
movl $0, (%eax)
# Function call
movl -4(%ebp), %eax
pushl %eax
movl $7, %eax
pushl %eax
call print -356
addl $8, %esp # Cleanup stack pushed arguments
# Function call
movl -4(%ebp), %eax
pushl %eax
movl $4096, %eax
pushl %eax
call free -377
addl $8, %esp # Cleanup stack pushed arguments
movl $0, %eax
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
