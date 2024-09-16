# Complete rewrite and extension of c4 (C in four functions) by Robert Swierczek

## Info
An exercise in minimalism.
Based on branch: switch-and-structs

Needs to be compiled with -m32 flag to work as int and pointers are 32 bit.

## Syntax


## Compile

Compiles to x86 machine code. 

## Usage
To compile 
```bash
make
```

```
Usage: ./cc [-o output_file] [-b] [-r] [-s] [--org 0x1000] [--elf] input_file
Options
  -o output_file: Specify output file
  -s: Print assembly
  --org 0x1000: Set the origin address
  --elf: Generate ELF file
  --ast: Print AST tree
```

Example:

```bash
./cc --elf -o output.o test.c
./output.o
```

## Changes
Because of the rewrite to be more readable and extendable, the code is not self compilable anymore.

### Include
Added include directive to include other files in the main file.
Files need to be included in the order they are used.

## Linux library
Includes a linux library in lib/linux.c

### Builtin functions

#### Used to call interrupts and I/O functions
```c
void __interrupt(int number, int eax, int ebx, int ecx, int edx);
```

#### Example of usage:

math.c:
```c
int add(int a, int b) {
    return a + b;
}
```

main.c:
```c
#include <math..h>

int main() {
    int a;
    int b;
    int c;

    a = 1;
    b = 2;
    c = add(a, b);
    return c;
}
```

### Data section

#### x86 m32
With x86 the Data section is located at the start of the file.
With ELF, its located after the ELF header.
However, the first 5 bytes are reserved for the JMP to main.
Access to the data section is dependent on the ORG. Note: Big global array declartions will bloat the binary.