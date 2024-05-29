# Complete rewrite and extension of c4 (C in four functions) by Robert Swierczek

## Info
An exercise in minimalism.
Based on branch: switch-and-structs

Needs to be compiled with -m32 flag to work as int and pointers are 32 bit.

## Compile

Compiles to bytecode or x86 machine code. 

## Usage
```bash
make
./c4 file.c
./c4 -r file.o
```

Or to use the demo
```bash
make demo
```


## Changes
Because of the rewrite to be more readable and extendable, the code is not self compilable anymore.

### Include
Added include directive to include other files in the main file.
Files need to be included in the order they are used.

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