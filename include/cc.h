#ifndef DF579CF4_EFA3_4966_AEE7_98CCF666A06B
#define DF579CF4_EFA3_4966_AEE7_98CCF666A06B

#include <unistd.h> /* For open, read, close */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h> /* For memcmp & strcmp */
#include <fcntl.h> /* For open */
#include <stdarg.h> /* For va_list */
#include <time.h>

#define POOL_SIZE 32*1024

enum TOKENS {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Break, Case, Char, Default, Else, Enum, If, Int, Return, Sizeof, Struct, Switch, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Dot, Arrow, Brak
};
enum OPCODES {
    LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE   ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT, IMD
};
enum { CHAR, INT, PTR = 256, PTR2 = 512 };

/**
 * @brief Struct to store the virtual machine state
 * Importantly, the machine has k of stack, and 65k of code.
 * Code and data access is relative to the code and data pointers.
 */
struct virtual_machine {
    /* Registers */
    int *pc;
    int *bp;
    int *sp;
    int ax;
    
    int *stack;
    
    /* sections */
    int *code;
    int *data;

    int cycle;
};

struct identifier {
    int tk;
    int hash;
    char *name;
    int name_length;
    int class;
    int type;
    int val;
    int stype;
    int hclass;
    int htype;
    int hval;
};

#define MAX_MEMBERS 128

struct member {
    struct identifier *ident;
    struct member *next;
    int offset;
    int type;
} ;

extern int *emitted_code;
extern int *last_emitted;

extern int *entry;

extern char *data;
extern char *org_data;

int* read_bytecode(const char *filename, size_t *code_size, char **data, size_t *data_size, int *main_pc);
void write_bytecode(const char *filename, int *code, size_t code_size, char *data, size_t data_size, int *main_pc);


#endif /* DF579CF4_EFA3_4966_AEE7_98CCF666A06B */
