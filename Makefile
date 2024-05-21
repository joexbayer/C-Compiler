# Makefile to compile and build cc.c
CCFLAGS = -Wall -g -m32
LDFLAGS = 
LD = ld
CC = gcc
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