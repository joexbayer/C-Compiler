# A custom C compiler to x86 32bit machine code.
## Mainly used as a submodule for RetrOS-32

### Building the Project

To build the project on Linux, you can use the provided `Makefile`. 

```sh
make
```

This creates the `cc` object file which is the compiler.
### Usage

To use the compiler, run the following command:

```sh
./cc input_file -o output_file
```

#### Options

- `input_file`: Must be the first argument!
- `-o output_file`: Specify the output file
- `--no-elf`: Do not generate ELF file (only available in Linux builds)
- `--org <address>`: Set origin address (only available in Linux builds)
- `-s`: Print assembly
- `--ast`: Print AST tree

By default ELF will be used if compile on Linux.

To clean up the build files, run:

```sh
make clean
```

### Quirks

This project currently supports `int` and `char` data types, as well as pointers and structs.
It also includes support for both global and local arrays.
Please note that local variables must be declared at the beginning of a function and initialized subsequently.
`#include` can include other .c files, which would be the same as pasting the code. (Order matters).
A file can only be included once, and wont be included again if the same `#include "file.c"` is used multiple places. 

#### Builtins

Currently supports __interrupt, __inportb __outportb builtins (Example in lib/linux.c)

### Struct Functions

The project now supports functions within structs. Here is an example of how to define and use struct functions:

```c
struct object {
  // Value needs to be declared before usage
  int value;

  int set(struct object *object, int value) {
    object->value = value;
    return 0;
  };

  int reset(struct object *object) {
    object->value = 0;
    return object->value;
  };

  int increment(struct object *object, int incrementValue) {
    object->value = object->value + incrementValue;
    return object->value;
  };

  int type;
};

int main() {
  struct object obj;
  obj.set(64);

  obj.increment(5);

  obj.reset();

  return obj.value;
}
```

This example demonstrates how to define functions within a struct and how to use them in the `main` function.

### Tests

```sh
make tests
```

### Examples

Checkout the files in /tests for examples.


## This project is a complete rewrite and inspired by c4 (C in four functions) by Robert Swierczek for RetrOS-32