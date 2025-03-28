#ifndef DF579CF4_EFA3_4966_AEE7_98CCF666A06B
#define DF579CF4_EFA3_4966_AEE7_98CCF666A06B

#ifdef NATIVE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void *zmalloc(int size);

#else

#include <args.h> /* For va_list */
#include <libc.h>
#include <lib/syscall.h>
#include <lib/printf.h>
#include <fs/fs.h>

#endif

#define POOL_SIZE 32*1024

#define ELF

enum TOKENS {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Break, Case, Char, Default, Else, Enum, If, Int, Return, Sizeof, Struct, Switch, While, Asm,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Dot, Arrow, BrakOpen, BrakClose
};

typedef enum loc_type {
    LOCAL_DEFINTION = 0,
    LOCAL_PARAMETER,
} loc_type_t;

enum __BUILTIN {
    INTERRUPT = 64,
    INPORTB,
    OUTPORTB,
    INPORTW,
    OUTPORTW,
    INPORTL,
    OUTPORTL,
    __UNUSED
};

enum OPCODES {
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT
};
enum { CHAR, INT, PTR = 256, PTR2 = 512 };

struct config {
    char *source;
    char *output;
    int run;
    int argc;
    char **argv;
    int assembly_set;
    int elf;
    int org;
    int ast;
};
extern struct config config;

struct identifier {
    int tk;
    int hash;
    char *name;
    int name_length;
    int class;
    int type;
    loc_type_t loc_type;
    int val;
    int stype;
    int hclass;
    int htype;
    int hval;
    int array; /* Array size */
    int array_type; /* Type of array */
    int args;
    unsigned int address;
};

#define MAX_MEMBERS 128

struct member {
    struct identifier *ident;
    struct member *next;
    int offset;
    int type;
};

extern int *entry;

extern char *data;
extern char *org_data;

int dbgprintf(const char *fmt, ...);
int write_elf_header(char* buffer, int entry, int text_size, int data_size);

//int compile_asm(char* asm, uint8_t* opcodes, int* opcodes_count);

#endif /* DF579CF4_EFA3_4966_AEE7_98CCF666A06B */
