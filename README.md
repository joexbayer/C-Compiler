# Complete rewrite and extension of c4 (C in four functions) by Robert Swierczek

## Info
An exercise in minimalism.
Based on branch: switch-and-structs

Needs to be compiled with -m32 flag to work as int and pointers are 32 bit.

## Compile

Compiles to bytecode or x86 machine code. 

## Usage
To compile 
```bash
make
```

```
Usage: ./cc [-o output_file] [-b] [-r] [-s] [--org 0x1000] [--elf] input_file
Options
  -o output_file: Specify output file
  -b: Generate bytecode
  -r: Run bytecode
  -s: Print assembly
  --org 0x1000: Set the origin address
  --elf: Generate ELF file
  --ast: Print AST tree
```

Example:

```bash
./cc -o output.o test.c
./output.o
```


```bash
./cc -b -o bytecode.o test.c
./cc -r bytecode.o
```

## Changes
Because of the rewrite to be more readable and extendable, the code is not self compilable anymore.

### Include
Added include directive to include other files in the main file.
Files need to be included in the order they are used.

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

### Bytecode output files
Added the possibility to output the bytecode to a file and run it later.
Mainly to avoid needing to recompile the code every time.

#### Position indepedent code and data access
The bytecode is position independent, so it can be loaded at any address in memory.
Only the relativ access to data and code is stored in the bytecode file.

#### Combined code and data in output.
The code and data sections are combined and written together to the output file.
Additionally, the size of the code and data sections are stored at the beginning of each section.
The entry point (main) is stored at the beginning of the file.