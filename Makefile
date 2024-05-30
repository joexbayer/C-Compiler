CCFLAGS = -Wall -g -m32 -Iinclude
LDFLAGS = 
LD = ld -m elf_i386
CC = gcc 
AS = as --32
MAKEFLAGS += --no-print-directory

OUTPUT = cc

SRC_DIR = src
OUTPUTDIR = ./bin/

SRC_FILES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OUTPUTDIR)%.o)

$(OUTPUT): $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) $(CCFLAGS)

$(OUTPUTDIR)%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OUTPUTDIR) $(dir $@)
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
	chmod 777 output.o

simple: $(OUTPUT)
	./$(OUTPUT) ./demo/simple.c
	chmod 777 output.o