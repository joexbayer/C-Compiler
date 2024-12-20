ROOT = ../../
CCFLAGS = -Wall -g -Iinclude -std=gnu11 -O1 -D__KERNEL -m32 \
		-Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-Wno-conversion -fno-omit-frame-pointer -I ./include/ -I $(ROOT)include/ -I $(ROOT)apps/
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
TESTS := $(wildcard ./tests/*)

.PHONY: all clean depend demo tests

all: $(OUTPUT) install

$(OUTPUT): $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) $(CCFLAGS) -T ./../utils/linker.ld -L../ -lcore

$(OUTPUTDIR)%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OUTPUTDIR) $(dir $@)
	$(CC) $(CCFLAGS) -c $< -o $@

install: $(OUTPUT)
	@cp $(OUTPUT) $(ROOT)rootfs/bin/$(OUTPUT).o

clean:
	rm -rf $(OUTPUTDIR)* *.o *.d $(OUTPUT) .depend

depend: .depend

.depend: $(SRC_FILES)
	rm -f ./.depend
	$(CC) $(CCFLAGS) -MM $^ > ./.depend;

include .depend