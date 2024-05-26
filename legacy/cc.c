/**
 * @file cc.c
 * @author Robert Swierczek, Joe Bayer (joexbayer)
 * @brief Simple C compiler/interpreter
 * @version 0.1
 * @date 2024-05-17
 * 
 * char, int, structs, and pointer types
 * if, while, return, switch, and expression statements
 *
 * Originally written by Robert Swierczek
 * Rewritten and modified for RetrOS-32 by Joe Bayer
 */

#include "cc.h"

#define POOL_SIZE 128*1024
#define MAX_MEMBERS 128

#define DEBUG
#undef DEBUG

#define IS_LETTER(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '_')
#define IS_DIGIT(x) (x >= '0' && x <= '9')
#define IS_HEX_DIGIT(x) (IS_DIGIT(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'))

enum TOKENS {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Break, Case, Char, Default, Else, Enum, If, Int, Return, Sizeof, Struct, Switch, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Dot, Arrow, Brak
};
enum OPCODES {
    LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT, IMD
};
enum { CHAR, INT, PTR = 256, PTR2 = 512 };


const char *opcodes = "LEA  IMM  JMP  JSR  BZ   BNZ  ENT  ADJ  LEV  LI   LC   SI   SC   PSH  OR   XOR  AND  EQ   NE   LT   GT   LE   GE   SHL  SHR  ADD  SUB  MUL  DIV  MOD  OPEN READ CLOS PRTF MALC MSET MCMP EXIT IMD";

static char *current_position;
static char *last_position;
static char *data;
static char *org_data;

static int *emitted_code;
static int *last_emitted;

static int *case_patch;
static int *break_patch;
static int *default_patch;

static int *type_size;
static int type_new = 0;

static int token;
static int ival;
static int type;
static int local_offset;

static int line;
static int src;
static int debug;

/**
 * @brief Pointers used to store the current position in the file
 * when another file is included. Having these as global variables
 * limits us to a single "depth" of file inclusion.
 */
static char *original_position;
static char *original_last_position;
static int original_line;
static char *include_buffer;

/**
 * @brief Struct to store the virtual machine state
 * Importantly, the machine has 65k of stack, and 65k of code.
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
static struct identifier *sym_table;
static struct identifier *last_identifier = {0};

struct member {
    struct identifier *ident;
    struct member *next;
    int offset;
    int type;
} *members[MAX_MEMBERS];

/* Prototypes */
int parse();
static void next();
static void expression(int level);
static void statement();
static void include(char *file);

int dbgprintf(const char *fmt, ...){
#ifdef DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
    return 0;
}

static void include(char *file) {
    int fd, len;
    dbgprintf("Including file: %s\n", file);

    /* Store the current parsing state */
    original_position = current_position + 1;
    original_last_position = last_position;
    original_line = line;

    fd = open(file, O_RDONLY);
    if (fd >= 0) {
        len = read(fd, include_buffer, POOL_SIZE - 1);
        if (len > 0) {
            include_buffer[len] = '\0';
            close(fd);

            /* Switch to new file */
            current_position = include_buffer;
            last_position = include_buffer + len;

            line = 1;
            next();
            parse();

            /* Restore the original parsing state */
            current_position = original_position;
            last_position = original_last_position;
            line = original_line;
            close(fd);
            return;
        }
    } else {
        printf("Unable to open include file: %s\n", file);
        exit(-1);
    }
}

static void next() {
    char *position;

    while((token = *current_position)){
        ++current_position;

        /* Check if new identifier*/
        if(IS_LETTER(token)){
            /* Store the current position */
            position = current_position - 1;

            /* Move to the end of the identifier */
            while(IS_LETTER(*current_position) || IS_DIGIT(*current_position)){
                token = token * 147 + *current_position++;
            }

             /* Hash the token and include the length */
            token = (token << 6) + (current_position - position);
            last_identifier = sym_table;

            /* Iterate over the symbol table to check for existing identifiers */
            while(last_identifier->tk){
                /* Compare the hash and name of the current identifier with the token */
                if(token == last_identifier->hash && !memcmp(last_identifier->name, position, current_position - position)){
                    token = last_identifier->tk;
                    return;
                }
                last_identifier = last_identifier + 1;
            }

            /* Store the name, hash, and token type of the new identifier */
            last_identifier->name = position;
            last_identifier->name_length = current_position - position;

            /* Null terminate name */
            last_identifier->hash = token;
            last_identifier->tk = Id;
            token = Id;
            return;

        } else if (IS_DIGIT(token)){
            /* Convert the token to an integer */
            if((ival = token - '0')){
                while (*current_position >= '0' && *current_position <= '9')
                    ival = ival * 10 + *current_position++ - '0'; 
            } else if(*current_position == 'x' || *current_position == 'X'){
                /* Hex */
                while((token = *++current_position) && IS_HEX_DIGIT(token)){
                    ival = ival * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                }
            } else {
                /* Octal */
                while(*current_position >= '0' && *current_position <= '7'){
                    ival = ival * 8 + *current_position++ - '0';
                }
            }
            token = Num;
            return;
        }

        /* Check for new line, spaces, tabs, etc */
        switch(token){
            case '\n':
                ++line;
            case ' ': case '\t': case '\v': case '\f': case '\r':
                break;
            /* Check for comments or division */
            case '/':
                if(*current_position == '/'){
                    while(*current_position != 0 && *current_position != '\n'){
                        ++current_position;
                    }
                } else {
                    token = Div;
                    return;
                }
                break;
            case '#':
               if (strncmp(current_position, "include", 7) == 0 && current_position[7] == ' ') {
                    current_position += 8; /* Move past "include " */
                    while (*current_position == ' ') current_position++;
                    if (*current_position == '"') {
                        char include_file[256], *start;
                        int len;
                        start = ++current_position;

                        while (*current_position != '"' && *current_position != '\0') current_position++;
                        if (*current_position == '"') {
                            len = current_position - start;
                            strncpy(include_file, start, len);
                            include_file[len] = '\0';

                            include(include_file);
                        }
                    }
                }
                while (*current_position != 0 && *current_position != '\n') {
                    ++current_position;
                }
                break;
            /* Check for string literals */
            case '"':
            case '\'':
                /* Write string to data */
                position = data;
                while (*current_position != 0 && *current_position != token) {
                    if ((ival = *current_position++) == '\\') {
                        switch (ival = *current_position++) {
                            case 'n': ival = '\n'; break;
                            case 't': ival = '\t'; break;
                            case 'v': ival = '\v'; break;
                            case 'f': ival = '\f'; break;
                            case 'r': ival = '\r';
                        }
                    }
                    *data++ = ival;
                }
                *data++ = 0; /* Null-terminate the string */
                ++current_position;
                if (token == '"') ival = (int) position; else token = Num;
                return;
            case '=':
                /* Check for equality or assignment */
                if(*current_position == '='){
                    ++current_position;
                    token = Eq;
                } else {
                    token = Assign;
                }
                return;
            case '+':
                if(*current_position == '+'){
                    ++current_position;
                    token = Inc;
                } else {
                    token = Add;
                }
                return;
            case '-':
                if(*current_position == '-'){
                    ++current_position;
                    token = Dec;
                /* Check for arrow or subtraction */
                } else if (*current_position == '>'){
                    ++current_position;
                    token = Arrow;
                } else {
                    token = Sub;
                }
                return;
            case '!':
                if(*current_position == '='){
                    ++current_position;
                    token = Ne;
                }
                return;
            case '<':
                if(*current_position == '='){
                    ++current_position;
                    token = Le;
                } else if(*current_position == '<'){
                    ++current_position;
                    token = Shl;
                } else {
                    token = Lt;
                }
                return;
            case '>':
                if(*current_position == '='){
                    ++current_position;
                    token = Ge;
                } else if(*current_position == '>'){
                    ++current_position;
                    token = Shr;
                } else {
                    token = Gt;
                }
                return;
            case '|':
                if(*current_position == '|'){
                    ++current_position;
                    token = Lor;
                } else {
                    token = Or;
                }
                return;
            case '&':
                if(*current_position == '&'){
                    ++current_position;
                    token = Lan;
                } else {
                    token = And;
                }
                return;
            case '^': token = Xor; return;
            case '%': token = Mod; return;
            case '*': token = Mul; return;
            case '[': token = Brak; return;
            case '?': token = Cond; return;
            case '.' : token = Dot; return;
            default:
                return;
        }
    }
}

/**
 * @brief Parse a expression
 * A expression is a sequence of terms separated by + or - operators
 * @param level depth of the expression
 */
static void expression(int level) {
    int t; /* Temporary register */
    int *b;
    int sz;

    struct identifier *id;
    struct member *m;

    switch(token){
        case 0: printf("%d: unexpected token EOF of expression\n", line); exit(-1);
        case Num:
            *++last_emitted = IMM;
            *++last_emitted = ival;
            next();
            type = INT;
            break;
        case '"':
            /* String */
            *++last_emitted = IMD;
            *++last_emitted = ival-(int)org_data; /* Relative address */
            next();
            while (token == '"') {
                next();
            }
            data = (char *)(((int)data + sizeof(int)) & -sizeof(int));
            type = PTR;
            break;
        case Sizeof:
            next();
            if(token == '('){
                next();
            }else {
                printf("%d: open parenthesis expected in sizeof\n", line);
                exit(-1);
            }

            type = INT;
            if(token == Int){
                next();
            } else if(token == Char){
                next();
                type = CHAR;
            } else if (token == Struct){
                next();
                if(token != Id){
                    printf("%d: bad struct type\n", line);
                    exit(-1);
                }
                t = last_identifier->stype;
                next();
                type = t;
            } 

            while(token == Mul){
                next();
                type += PTR;
            }

            if(token == ')'){
                next();
            } else {
                printf("%d: close parenthesis expected in sizeof\n", line);
                exit(-1);
            }

            *++last_emitted = IMM;
            *++last_emitted = type >= PTR ? sizeof(int) : type_size[type];
            type = INT;
            break;
        case Id:
            id = last_identifier;
            next();

            /* Check for function call */
            if(token == '('){
                /* Function call */
                next();
                t = 0;

                /* Parse the arguments */
                while(token != ')'){
                    expression(Assign);
                    *++last_emitted = PSH;

                    /* t is the number of arguments */
                    t++;
                    if(token == ','){
                        next();
                    }
                }
                next();

                /* Check if the identifier is a system function */
                if(id->class == Sys){
                    *++last_emitted = id->val;
                } else if(id->class == Fun){
                    *++last_emitted = JSR;
                    *++last_emitted = id->val - (int)emitted_code;
                } else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }

                if(t){
                    *++last_emitted = ADJ;
                    *++last_emitted = t;
                }

                type = id->type;
                
            } else if (id->class == Num){
                /* Check for a number */
                *++last_emitted = IMM;
                *++last_emitted = id->val;
                type = id->type;
            } else {
                if(id->class == Loc){
                    /* Local variable */
                    *++last_emitted = LEA;
                    *++last_emitted = local_offset - id->val;
                    printf("Local offset: %d\n", local_offset - id->val);
                } else if(id->class == Glo){
                    /* Global variable */
                    *++last_emitted = IMD;
                    *++last_emitted = id->val - (int)org_data;
                } else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }
                
                printf("Type: %d\n", id->type);
                if ((type = id->type) <= INT || type >= PTR){
                    *++last_emitted = (type == CHAR) ? LC : LI;

                }

            }
            break;
        case '(':
            next();
            if(token == Int || token == Char || token == Struct){
                /* Cast */
                if(token == Int){
                    next();
                    t = INT;
                } else if(token == Char){
                    next();
                    t = CHAR;
                } else {
                    next();
                    if (token != Id){
                        printf("%d: bad struct type: %d\n", line, token);
                        exit(-1);
                    }
                    t = last_identifier->stype;
                    next();
                }

                /* Check for pointers */
                while(token == Mul){
                    next();
                    t += PTR;
                }

                /* Check for the end of the cast */
                if(token == ')'){
                    next();
                } else {
                    printf("%d: bad cast: %c (%d)\n", line, token, token);
                    exit(-1);
                }
                
                /* Parse the expression */
                expression(Inc);
                type = token;
            } else {
                /* Check for a sub-expression */
                expression(Assign);
                if(token == ')'){
                    next();
                } else {
                    printf("%d: bad expression\n", line);
                    exit(-1);
                }
            }
            break;
        
        case Mul: /* Unary operators '*' */
            next();
            expression(Inc);

            /* Check for pointers */
            if(type > INT){
                type -= PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            /* Check for the end of the dereference */
            if( type <= INT || type >= PTR){
                *++last_emitted = (type == CHAR) ? LC : LI;
            }
            break; 
        case And: /* Unary operators '&' */
            next();
            expression(Inc);

            if(*last_emitted == LC || *last_emitted == LI){
                --last_emitted;
            } 

            type = type + PTR;
            break;
        
        case '!': /* Unary operators '!' */
            next();
            expression(Inc);

            /* if(*last_emitted == LC || *last_emitted == LI){
                last_emitted--;
            } */

            *++last_emitted = PSH;
            *++last_emitted = IMM;
            *++last_emitted = 0;
            *++last_emitted = EQ;
            type = INT;
            break;

        case '~': /* Unary operators '~' */
            next();
            expression(Inc);
            *++last_emitted = PSH;
            *++last_emitted = IMM;
            *++last_emitted = -1;
            *++last_emitted = XOR;
            type = INT;
            break;

        case Add: /* Unary operators '+' */
            next();
            expression(Inc);
            type = INT;
            break;
        
        case Sub: /* Unary operators '-' */
            next();

            *++last_emitted = IMM;

            if(token == Num){
                *++last_emitted = -ival;
                next();
            } else {
                *++last_emitted = -1; /* 1 * expression */
                *++last_emitted = PSH;
                expression(Inc);
                *++last_emitted = MUL;
            }
            type = INT;
            break;

        case Inc: /* Unary operators '++' */
        case Dec: /* Unary operators '--' */
            t = token;
            next();
            expression(Inc);

            if(*last_emitted == LC){
                *last_emitted = PSH;
                *++last_emitted = LC;
            } else if(*last_emitted == LI){
                *last_emitted = PSH;
                *++last_emitted = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line);
                exit(-1);
            }

            *++last_emitted = PSH;
            *++last_emitted = IMM;
            *++last_emitted = type >= PTR2 ? sizeof(int) : (type >= PTR) ? type_size[type - PTR] : 1;
            //*++last_emitted = (type > PTR) ? type_size[type] : sizeof(int);
            *++last_emitted = (t == Inc) ? ADD : SUB;
            *++last_emitted = (type == CHAR) ? SC : SI;
            break;
        default:
            printf("%d: bad expression 2\n", line);
            exit(-1);
    }
    
    while( token >= level ){
        t = type;
    
        switch(token){
            case Assign:
                next();

                if (*last_emitted == LC || *last_emitted == LI){
                    *last_emitted = PSH;
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                expression(Assign);
                *++last_emitted = ((type = t) == CHAR) ? SC : SI;
                break;
            case Cond:
                next();
                *++last_emitted = BZ;
                b = ++last_emitted;

                expression(Assign);
                if(token == ':'){
                    next();
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }

                /* Check for the end of the conditional */
                *b = (int) ((int)(last_emitted+3) - (int)emitted_code);
                *++last_emitted = JMP;
                b = ++last_emitted;

                expression(Cond);
                *b = (int) ((int)(last_emitted+1) - (int)emitted_code);
                break;
            case Lor:
                next();
                *++last_emitted = BNZ;
                b = ++last_emitted;

                expression(Lan);
                *b = (int) ((int)(last_emitted+1) - (int)emitted_code);
                type = INT;
                break;
            case Lan:
                next();
                *++last_emitted = BZ;
                b = ++last_emitted;

                expression(Or);
                *b = (int) ((int)(last_emitted+1) - (int)emitted_code);
                type = INT;
                break;
            
            case Or:
                next();
                *++last_emitted = PSH;
                expression(Xor);
                *++last_emitted = OR;
                type = INT;
                break;

            case Xor:
                next();
                *++last_emitted = PSH;
                expression(And);
                *++last_emitted = XOR;
                type = INT;
                break;

            case And:
                next();
                *++last_emitted = PSH;
                expression(Eq);
                *++last_emitted = AND;
                type = INT;
                break;
            
            case Eq:
                next();
                *++last_emitted = PSH;
                expression(Lt);
                *++last_emitted = EQ;
                type = INT;
                break;
            
            case Ne:
                next();
                *++last_emitted = PSH;
                expression(Lt);
                *++last_emitted = NE;
                type = INT;
                break;
            
            case Lt:
                next();
                *++last_emitted = PSH;
                expression(Shl);
                *++last_emitted = LT;
                type = INT;
                break;
            
            case Gt:
                next();
                *++last_emitted = PSH;
                expression(Shl);
                *++last_emitted = GT;
                type = INT;
                break;
            
            case Le:
                next();
                *++last_emitted = PSH;
                expression(Shl);
                *++last_emitted = LE;
                type = INT;
                break;
            
            case Ge:
                next();
                *++last_emitted = PSH;
                expression(Shl);
                *++last_emitted = GE;
                type = INT;
                break;
            
            case Shl:
                next();
                *++last_emitted = PSH;
                expression(Add);
                *++last_emitted = SHL;
                type = INT;
                break;
            
            case Shr:
                next();
                *++last_emitted = PSH;
                expression(Add);
                *++last_emitted = SHR;
                type = INT;
                break;
            
            case Add:
                next();
                *++last_emitted = PSH;
                expression(Mul);

                /* Check for pointers */
                sz = (type = t) >= PTR2 ? sizeof(int) : (type >= PTR) ? type_size[type - PTR] : 1;
                if(sz > 1){
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = sz;
                    *++last_emitted = MUL;
                }
                *++last_emitted = ADD;

                break;
            
            case Sub:
                next();
                *++last_emitted = PSH;
                expression(Mul);

                sz = t >= PTR2 ? sizeof(int) : t >= PTR ? type_size[t - PTR] : 1;
                if(t == type && sz > 1){
                    *++last_emitted = SUB;
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = sz;
                    *++last_emitted = DIV;
                    type = INT;
                } else if (sz > 1){
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = sz;
                    *++last_emitted = MUL;
                    *++last_emitted = SUB;
                } else {
                    *++last_emitted = SUB;
                }
                type = t;
                break; 
            case Mul:
                next();
                *++last_emitted = PSH;
                expression(Inc);
                *++last_emitted = MUL;
                type = INT;
                break;

            case Div:
                next();
                *++last_emitted = PSH;
                expression(Inc);
                *++last_emitted = DIV;
                type = INT;
                break;
            
            case Mod:
                next();
                *++last_emitted = PSH;
                expression(Inc);
                *++last_emitted = MOD;
                type = INT;
                break;
            
            case Inc: case Dec:
                if(*last_emitted == LC){
                    *last_emitted = PSH;
                    *++last_emitted = LC;
                } else if(*last_emitted == LI){
                    *last_emitted = PSH;
                    *++last_emitted = LI;
                } else {
                    printf("%d: bad lvalue in post-increment\n", line);
                    exit(-1);
                }

                sz = (type > PTR2) ? sizeof(int) : (type > PTR) ? type_size[type - PTR] : 1;

                *++last_emitted = PSH;
                *++last_emitted = IMM;
                *++last_emitted = sz;
                *++last_emitted = (token == Inc) ? ADD : SUB;
                *++last_emitted = (type == CHAR) ? SC : SI;
                *++last_emitted = PSH;
                *++last_emitted = IMM;
                *++last_emitted = sz;
                *++last_emitted = (token == Inc) ? SUB : ADD;
                next();
                break;

            case Dot:
                type += PTR;
                // fall through to Arrow case

            case Arrow:
                if (type <= PTR + INT || type >= PTR2) {
                    printf("%d: illegal use of ->\n", line);
                    exit(-1);
                }
                next();
                if (token != Id) {
                    printf("%d: illegal use of ->\n", line);
                    exit(-1);
                }
                m = members[type - PTR];
                while (m && m->ident != last_identifier) {
                    m = m->next;
                }
                if (!m) {
                    printf("%d: struct member not found\n", line);
                    exit(-1);
                }
                if (m->offset) {
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = m->offset;
                    *++last_emitted = ADD;
                }
                type = m->type;
                if (type <= INT || type >= PTR) {
                    *++last_emitted = (type == CHAR) ? LC : LI;
                }
                next();
                break;

            
            case Brak:
                next();
                *++last_emitted = PSH;
                expression(Assign);
                if(token == ']'){
                    next();
                } else {
                    printf("%d: close bracket expected\n", line);
                    exit(-1);
                }

                if (t < PTR){
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                sz = (t = type - PTR) >= PTR2 ? sizeof(int) : type_size[t - PTR];
                if(sz > 1){
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = sz;
                    *++last_emitted = MUL;
                }
                *++last_emitted = ADD;

                if((type = t) <= INT || type >= PTR){
                    *++last_emitted = (type == CHAR) ? LC : LI;
                } else {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                break;
            default:
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
        }
    }
}

static void statement() {

    /* Temporary registers */
    int *a, *b, *d, i;

    switch(token){
        case If:
            next();
            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();

            /* Parse the expression inside if () */
            expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();
            *++last_emitted = BZ;
            b = ++last_emitted;

            /* Parse the statement inside if */
            statement();

            if(token == Else){
                /* Parse the statement inside else */
                *b = (int) ((int)(last_emitted+3) - (int)emitted_code);
                *++last_emitted = JMP;
                b = ++last_emitted;

                next();
                statement();
            }

            *b = (int) ((int)(last_emitted+1) - (int)emitted_code);
            return;
        
        case While: 
            next();
            a = last_emitted+1;

            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();
            
            /* Parse the expression inside while () */
            expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();

            *++last_emitted = BZ;
            b = ++last_emitted;

            /* Parse the statement inside while */
            statement();

            *++last_emitted = JMP;
            *++last_emitted = (int)((int)a - (int)emitted_code);
            *b = (int) ((int)(last_emitted+1) - (int)emitted_code);
            return;

        case Switch:
            next();
            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();

            expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();

            /* case_patch is not initilized? */
            a = case_patch;
            *++last_emitted = JMP;
            case_patch = ++last_emitted;

            b = break_patch;
            d = default_patch;
            break_patch = default_patch = 0;

            statement();

            *case_patch = default_patch ? (int)default_patch - (int)emitted_code : (int)(last_emitted + 1) - (int)emitted_code;
            
            case_patch = a;

            while(break_patch){
                a = (int*) (*break_patch + (int)emitted_code);
                *break_patch = (int) (last_emitted + 1) - (int) emitted_code;
                break_patch = (int*)a;
            }

            break_patch = b;
            default_patch = d;
            return;
        
        case Case:
            *++last_emitted = JMP;
            ++last_emitted;
            *last_emitted = (int)(last_emitted + 7) - (int)emitted_code;
            *++last_emitted = PSH;

            i = *case_patch;
            *case_patch = (int)last_emitted - (int)emitted_code;

            next();
            expression(Or);

            if(last_emitted[-1] != IMM){
                printf("%d: case value must be constant\n", line);
                exit(-1);
            }

            *last_emitted = *last_emitted - i;
            *++last_emitted = SUB;
            *++last_emitted = BNZ;
            case_patch = ++last_emitted;
            *last_emitted = i + last_emitted[-3];

            if(token != ':'){
                printf("%d: colon expected\n", line);
                exit(-1);
            }
            next();

            statement();
            return;
        
        case Break:
            next();
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();

            *++last_emitted = JMP;
            *++last_emitted = (int) break_patch - (int) emitted_code;
            break_patch = last_emitted;
            return;

        case Default:
            next();
            if(token != ':'){
                printf("%d: colon expected\n", line);
                exit(-1);
            }
            next();

            default_patch = last_emitted + 1;

            statement();
            return;

        case Return:
            next();
            if(token != ';'){
                expression(Assign);
            }
            *++last_emitted = LEV;
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();

            return;

        case '{':
            next();
            while(token != '}'){
                statement();
            }
            next();
            return;
        
        case ';':
            next();
            return;
        
        default:
            expression(Assign);
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();

    }

}

void print_code(int *code, size_t code_size, char *data, size_t data_size) {
    printf("Machine Code:\n");
    size_t i = 0;
    while (i < code_size) {
        int opcode = code[i++];
        char *label = NULL;
        int label_len = 0;

        for (struct identifier *id = sym_table; id->tk; id++) {
            if (id->class == Fun && id->val == (int)(code + i)) {
                label = id->name;
                label_len = id->name_length;
                break;
            }
        }
        if (label) {
            printf("%.*s:\n", label_len, label);
        }

        if (opcode <= ADJ) {
            printf("  %.4s %d\n", &opcodes[opcode * 5], code[i++]);
        } else {
            printf("  %.4s\n", &opcodes[opcode * 5]);
        }
    }
}

int parse(){
    int i;
    int bt;
    int ty;
    int mbt;
    struct member *m;

    while(token){
        bt = INT;

        if(token == Int) next();
        else if(token == Char){
            next();
            bt = CHAR;
        } else if(token == Enum){
            next();
            if(token != '{'){
                next();
            }
            if(token == '{'){
                next();
                i = 0;
                while(token != '}'){
                    if(token != Id){
                        printf("%d: bad enum identifier %d\n", line, token);
                        exit(-1);
                    }
                    next();
                    if(token == Assign){
                        next();
                        if(token != Num){
                            printf("%d: bad enum initializer\n", line);
                            exit(-1);
                        }
                        i = ival;
                        next();
                    }
                    last_identifier->class = Num;
                    last_identifier->type = INT;
                    last_identifier->val = i++;
                    if(token == ','){
                        next();
                    }
                }
                next();
            }
        } else if( token == Struct ) {
            next();
            if(token == Id){
               if(!last_identifier->stype){
                   last_identifier->stype = type_new++;
               }
               bt = last_identifier->stype;
                next();
            } else {
                bt = type_new++;
            }

            if(token == '{'){
                next();
                if(members[bt]){
                    printf("%d: duplicate struct definition\n", line);
                    exit(-1);
                }

                i = 0;

                while(token != '}'){
                    mbt = INT;
                    if(token == Int) next();
                    else if(token == Char){
                        next();
                        mbt = CHAR;
                    } else if(token == Struct){
                        next();
                        if(token != Id){
                            printf("%d: bad struct member\n", line);
                            exit(-1);
                        }
                        mbt = last_identifier->stype;
                        next();
                    }

                    while(token != ';'){
                        ty = mbt;
                        while(token == Mul){
                            next();
                            ty = ty + PTR;
                        }

                        if(token != Id){
                            printf("%d: bad struct member\n", line);
                            exit(-1);
                        }

                        m = (struct member*) malloc(sizeof(struct member));
                        if(!m){
                            printf("Unable to malloc struct member\n");
                            exit(-1);
                        }

                        m->ident = last_identifier;
                        m->type = ty;
                        m->offset = i;
                        m->next = members[bt];
                        members[bt] = m;

                        i = i + (ty >= PTR ? sizeof(int) : type_size[ty]);
                        i = (i + 3) & -4;
                        
                        next();
                        if(token == ','){
                            next();
                        }
                    }
                    next();
                }
                next();
                type_size[bt] = i;
            }
        }
        while(token != ';' && token != '}'){
            ty = bt;
            while(token == Mul){
                next();
                ty = ty + PTR;
            }

            if(token != Id){
                printf("%d: bad global declaration\n", line);
                exit(-1);
            }

            if(last_identifier->class){
                printf("%d: duplicate global definition, %d\n", line, last_identifier->class);
                exit(-1);
            }

            next();

            last_identifier->type = ty;

            /* Check for function */
            if(token == '('){
                last_identifier->class = Fun;
                last_identifier->val = (int) (last_emitted + 1);
                next();

                i = 0;
                while(token != ')'){
                    ty = INT;
                    if(token == Int){
                        next();
                    } else if(token == Char){
                        next();
                        ty = CHAR;
                    } else if(token == Struct){
                        next();
                        if(token != Id){
                            printf("%d: bad struct type\n", line);
                            exit(-1);
                        }
                        ty = last_identifier->stype;
                        next();
                    }

                    while(token == Mul){
                        next();
                        ty = ty + PTR;
                    }

                    if(token != Id){
                        printf("%d: bad function definition\n", line);
                        exit(-1);
                    }

                    if(last_identifier->class == Loc){
                        printf("%d: duplicate parameter definition\n", line);
                        exit(-1);
                    }

                    last_identifier->hclass = last_identifier->class;
                    last_identifier->htype = last_identifier->type;
                    last_identifier->hval = last_identifier->val;
                    last_identifier->class = Loc;
                    last_identifier->type = ty;
                    last_identifier->val = i++;

                    next();

                    if(token == ','){
                        next();
                    }
                }
                next();

                if(token != '{'){
                    printf("%d: bad function definition\n", line);
                    exit(-1);
                }
                
                local_offset = ++i;

                next();

                while(token == Int || token == Char || token == Struct){
                    if (token == Int) bt = INT;
                    else if (token == Char) bt = CHAR;
                    else {
                        next();
                        if (token != Id) {
                            printf("%d: bad struct type\n", line);
                            exit(-1);
                        }
                        bt = last_identifier->stype;
                    }
                    next();
                    while (token != ';') {
                        ty = bt;
                        while (token == Mul) {
                            next();
                            ty = ty + PTR;
                        }
                        if (token != Id) {
                            printf("%d: bad local declaration\n", line);
                            exit(-1);
                        }
                        if (last_identifier->class == Loc) {
                            printf("%d: duplicate local definition\n", line);
                            exit(-1);
                        }
                        last_identifier->htype = last_identifier->type;
                        last_identifier->hclass = last_identifier->class;
                        last_identifier->hval = last_identifier->val;
                        last_identifier->type = ty;
                        last_identifier->class = Loc;
                        /* Allocate space based on size */
                        i = i + (ty >= PTR ? sizeof(int) : type_size[ty]);
                        last_identifier->val = i;
                           
                        next();
                        if (token == ',') {
                            next();
                        }
                    }
                    next();
                }

                *++last_emitted = ENT;
                *++last_emitted = i - local_offset;


                while(token != '}'){
                    statement();
                }
                *++last_emitted = LEV;

                last_identifier = sym_table;
                while(last_identifier->tk){
                    if(last_identifier->class == Loc){
                        last_identifier->class = last_identifier->hclass;
                        last_identifier->type = last_identifier->htype;
                        last_identifier->val = last_identifier->hval;
                    }
                    last_identifier = last_identifier + 1;
                }
            } else {
                last_identifier->class = Glo;
                last_identifier->val = (int)data;
                /* Allocate space based on size */
                data += (ty >= PTR ? sizeof(int) : type_size[ty]);

            }
            if(token == ','){
                next();
            } 
        }
        next();
    }
    return 0;
}

void run_virtual_machine(int *pc, int* code, char *data, int argc, char *argv[]) {
    int a, cycle = 0;
    int i, *t;
    int *sp, *bp = NULL;
    int *org;

    org = sp = (int *)malloc(POOL_SIZE);
    if (!sp) {printf("Unable to malloc sp\n"); exit(-1);}
    memset(sp, 0, POOL_SIZE);

    bp = sp;

    /* Set up the stack */
    sp = (int *)((int)sp + POOL_SIZE);
    *--sp = EXIT;
    *--sp = PSH;
    t = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)t;

    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    while (1) {
        i = *pc++;
        ++cycle;
        
        if (1) {
            printf("%d> %.4s", cycle, &opcodes[i * 5]);
            if (i <= ADJ)
                printf(" %d\n", *pc);
            else
                printf("\n");
        }

        switch (i) {
            case LEA:  {
                    /* Load local address based on offset in *pc */
                    a = (int)(bp + *pc++);
                }
                break; /* load local addresss */
            case IMD:  a = (int)(data + *pc++); break; /* load relativ data address */
            case IMM:  a = *pc++; break;             /* load global address or immediate */
            case JMP:  pc = (int *)((int)code + *pc); break;       /* jump */
            case JSR:  {
                    /* "push" return address and jump to offset in *pc */
                    *--sp = (int)(pc + 1);
                    pc = (int *)((int)code + *pc);
                }
                break; /* jump to subroutine */
            case BZ:   pc = a ? pc + 1 : (int *)((int)code + *pc); break; /* branch if zero */
            case BNZ:  pc = a ? (int *)((int)code + *pc) : pc + 1; break; /* branch if not zero */
            case ENT: {
                    /* Setup bp and sp, allocate space for local variables */
                    *--sp = (int)bp;
                    bp = sp;
                    sp = sp - *pc++;
                }
                break; /* enter subroutine */
            case ADJ:  sp = sp + *pc++; break; /* stack adjust */
            case LEV:  {
                    /* Restore bp and sp */
                    sp = bp;
                    bp = (int *)*sp++;
                    pc = (int *)*sp++;                
                }
                break; /* leave subroutine */
            case LI:   a = *(int *)a; break; /* load int */
            case LC:   a = *(char *)a; break; /* load char */
            case SI:   *(int *)*sp++ = a; break; /* store int */
            case SC:   a = *(char *)*sp++ = a; break; /* store char */
            case PSH:  *--sp = a; break; /* push */

            case OR:   a = *sp++ |  a; break;
            case XOR:  a = *sp++ ^  a; break;
            case AND:  a = *sp++ &  a; break;
            case EQ:   a = *sp++ == a; break;
            case NE:   a = *sp++ != a; break;
            case LT:   a = *sp++ <  a; break;
            case GT:   a = *sp++ >  a; break;
            case LE:   a = *sp++ <= a; break;
            case GE:   a = *sp++ >= a; break;
            case SHL:  a = *sp++ << a; break;
            case SHR:  a = *sp++ >> a; break;
            case ADD:  a = *sp++ +  a; break;
            case SUB:  a = *sp++ -  a; break;
            case MUL:  a = *sp++ *  a; break;
            case DIV:  a = *sp++ /  a; break;
            case MOD:  a = *sp++ %  a; break;

            case OPEN: a = open((char *)sp[1], *sp); break;
            case READ: a = read(sp[2], (char *)sp[1], *sp); break;
            case CLOS: a = close(*sp); break;
            case PRTF: t = sp + pc[1];a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); break;
            case MALC: a = (int)malloc(*sp); break;
            case MSET: a = (int)memset((char *)sp[2], sp[1], *sp); break;
            case MCMP: a = memcmp((char *)sp[2], (char *)sp[1], *sp); break;
            case EXIT: {
                printf("exit(%d) cycle = %d\n", *sp, cycle);
                goto vm_exit;
            }
            default: printf("cc: unknown instruction = %d! cycle = %d\n", i, cycle); goto vm_exit;
        }
    }
vm_exit:
    ;
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);

    printf("Execution time = %ld ns\n", diffInNanos);
}




int print_symbols() {
    printf("Symbols:\n");
    for (struct identifier *id = sym_table; id->tk; id++) {
        printf("  %.*s: tk=%d class=%d type=%d val=%d\n", id->name_length, id->name, id->tk, id->class, id->type, id->val);
    }
    return 0;
}

int run_byte_code(char* filename, int argc, char *argv[]){
    printf("Running bytecode from %s\n", filename);
    
    char *data;
    int main_offset;
    size_t code_size, data_size;
    int *pc = read_bytecode(filename, &code_size, &data, &data_size, &main_offset);
    int *main_pc = (int*)((int)pc + main_offset);

    printf("Running virtual machine\n");
    run_virtual_machine(main_pc, pc, data, argc - 2, &argv[2]);

    /* free */
    free(pc);
    free(data);

    return 0;
}

void compile_and_run(char* filename, int argc, char *argv[]){
    
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    int fd;
    struct identifier *main_identifier;
    const char *output_file = "a.out";

    const char *source_file = argv[1];
    if ((fd = open(source_file, 0)) < 0) {
        printf("Unable to open source file: %s\n", source_file);
        exit(-1);
    }

    /* Allocate memory */
    sym_table = (struct identifier *)malloc(POOL_SIZE);
    if (!sym_table) {printf("Unable to malloc sym_table\n");exit(-1);}
    last_emitted = emitted_code = (int *)malloc(POOL_SIZE);
    if (!emitted_code) {printf("Unable to malloc emitted_code\n");exit(-1);}
    org_data = data = (char *)malloc(POOL_SIZE);
    if (!data) {printf("Unable to malloc data\n");exit(-1);}
    type_size = (int *)malloc(PTR * sizeof(int));
    if (!type_size) {printf("Unable to malloc type_size\n");exit(-1);}
    include_buffer = (char *)malloc(POOL_SIZE);
    if (!include_buffer) {printf("Unable to malloc include_buffer\n");exit(-1);}

    memset(sym_table, 0, POOL_SIZE);
    memset(members, 0, MAX_MEMBERS * sizeof(struct member *));
    memset(emitted_code, 0, POOL_SIZE);
    memset(data, 0, POOL_SIZE);
    memset(type_size, 0, PTR * sizeof(int));

    current_position = "break case char default else enum if int return sizeof struct switch while "
                       "open read close printf malloc memset memcmp exit void main";

    /* Read in symbols */
    int i = Break;
    while (i <= While) {
        next();
        last_identifier->tk = i++;
    }

    /* Read in keywords */
    i = OPEN;
    while (i <= EXIT) {
        next();
        last_identifier->class = Sys;
        last_identifier->type = INT;
        last_identifier->val = i++;
    }
    next();
    last_identifier->tk = Char;
    next();
    main_identifier = last_identifier;

    /* Read in src file */
    if (!(last_position = current_position = malloc(POOL_SIZE))) {
        printf("could not malloc(%d) source area\n", POOL_SIZE);
        exit(-1);
    }

    if ((i = read(fd, current_position, POOL_SIZE - 1)) <= 0) {
        printf("read() returned %d\n", i);
        exit(-1);
    }

    current_position[i] = 0;
    close(fd);

    type_size[type_new++] = sizeof(char);
    type_size[type_new++] = sizeof(int);

    /* Parse the source code */
    line = 1;
    next();
    parse();

    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);

    int *pc;
    if (!(pc = (int *)main_identifier->val)) {
        printf("main() not defined\n");
        exit(-1);
    }

    printf("PC = %p, %p\n", pc, emitted_code);
    
    --argc; ++argv;

    // Print args
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    run_virtual_machine(pc, emitted_code, org_data, argc, argv);
    printf("Compilation time: %ld ns\n", diffInNanos);

    size_t code_size = last_emitted - emitted_code;
    size_t data_size = data - org_data;
    int main_offset = (int)(main_identifier->val - (int)emitted_code);
    write_bytecode(output_file, emitted_code, code_size, org_data, data_size, &main_offset);
    printf("Compilation successful. Machine code written to %s\n", output_file);

    //print_code(emitted_code, last_emitted - emitted_code, org_data, data - org_data);
    //print_symbols();

    free(sym_table);
    free(emitted_code);
    free(org_data);
    free(type_size);
    free(include_buffer);
    free(last_position);
}

int main(int argc, char *argv[]) {
   
    char *run_file = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                run_file = argv[++i];
            } else {
                printf("Usage: %s [-o <output_file>] [-r <run_file>] <source_file>\n", argv[0]);
                return -1;
            }
        }
    }

    if (run_file) {
        run_byte_code(run_file, argc, argv);
        return 0;
    }

    compile_and_run(0, argc, argv);

    return 0;
}
