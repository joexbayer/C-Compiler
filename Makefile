CCFLAGS = -Wall -ggdb -Iinclude -std=gnu11 -O1 -DNATIVE -Wall -Wextra -Wpedantic -Wstrict-aliasing
LDFLAGS = 
LD = ld
CC = gcc 
AS = as
MAKEFLAGS += --no-print-directory

OUTPUT = cc

SRC_DIR = src
OUTPUTDIR = ./bin/

SRC_FILES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OUTPUTDIR)%.o)
TESTS := $(wildcard ./tests/*)

.PHONY: all clean depend demo tests

all: $(OUTPUT)

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

os: cc
	./cc --org 0x10000 --no-elf playground/os.c
	make -C playground/os
	qemu-system-i386.exe playground/os/image.iso -d cpu_reset -D ./log.txt
	qemu-system-i386 playground/os/image.iso -d cpu_reset -D ./log.txt

tests: $(OUTPUT)
	@for file in $(TESTS); do \
		echo "[TEST $$file]"; \
		rm -f a.out; \
		./$(OUTPUT) $$file; \
		./a.out; \
	done