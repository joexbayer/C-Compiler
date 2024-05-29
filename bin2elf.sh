#!/bin/bash

# This script converts a binary file to an ELF file
# Usage: bin2elf.sh <binary file> <ELF file>

if [ $# -ne 2 ]; then
    echo "Usage: $0 <binary file> <ELF file>"
    exit 1
fi

# Create a new ELF file
objcopy --input-target=binary --output-target=elf32-i386 --binary-architecture=i386 $1 $1.o
ld -m elf_i386 -o $2 $1.o -T binary.ld
rm $1.o

