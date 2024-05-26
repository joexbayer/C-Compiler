# Makefile to compile and build cc.c
CCFLAGS = -Wall -g -m32
LDFLAGS = 
LD = ld -m elf_i386
CC = gcc 
AS = as --32
MAKEFLAGS += --no-print-directory

OUTPUT = cc

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:%.c=$(OUTPUTDIR)%.o)
OUTPUTDIR = ./bin/

$(OUTPUT): $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) $(CCFLAGS)

$(OUTPUTDIR)%.o: %.c
	@mkdir -p $(OUTPUTDIR)
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUTDIR)* *.o *.d $(OUTPUT) .depend

depend: .depend

.depend: $(SRC_FILES)
	rm -f ./.depend
	$(CC) $(CCFLAGS) -MM $^ > ./.depend;

include .depend

demo: $(OUTPUT)
	./$(OUTPUT) ./demo/main.c

simple: $(OUTPUT)
	./$(OUTPUT) ./demo/simple.c

experiment:
	gcc -m32 -o exp experiments/cc_ast.c io.c -g
	./exp ./demo/simple.c

assembly:
	$(AS) -o a.o a.s
	$(LD) -o a a.o -lc -dynamic-linker /lib/ld-linux.so.2 -e _start -L/usr/lib -L/lib -l:libc.so.6
	./a
	echo $?

asm:
	gcc -S -m32 demo/simple.c