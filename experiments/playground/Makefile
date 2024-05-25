# Makefile for building a simple GAS assembly program

# Define the assembler and linker
AS = as --32
LD = ld -m elf_i386

# Define the source and target files
SRC = main.s
OBJ = main.o
TARGET = hello

# Default target to build the program
all: $(TARGET)

# Rule to assemble the .s file into an object file
$(OBJ): $(SRC)
	$(AS) -o $(OBJ) $(SRC)

# Rule to link the object file into an executable
$(TARGET): $(OBJ)
	$(LD) -o $(TARGET) $(OBJ) -lc -dynamic-linker /lib/ld-linux.so.2 -e _start -L/usr/lib -L/lib -l:libc.so.6

# Rule to clean the build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Rule to run the program
run: $(TARGET)
	./$(TARGET)
