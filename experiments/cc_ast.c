/**
 * @file cc.c
 * @author Robert Swierczek, Joe Bayer (joexbayer)
 * @brief Simple C compiler/interpreter with AST
 * @version 0.2
 * @date 2024-05-17
 * 
 * char, int, structs, and pointer types
 * if, while, return, switch, and expression statements
 *
 * Originally written by Robert Swierczek
 * Rewritten and modified for RetrOS-32 by Joe Bayer
 */

#include "../cc.h"

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
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE   ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT, IMD
};
enum { CHAR, INT, PTR = 256, PTR2 = 512 };

const char *opcodes = "LEA  IMM  JMP  JSR  BZ   BNZ  ENT  ADJ  LEV  LI   LC   SI   SC   PSH  OR   XOR  AND  EQ   NE   LT   GT   LE   GE   SHL  SHR  ADD  SUB  MUL  DIV  MOD  OPEN READ CLOS PRTF MALC MSET MCMP EXIT IMD";

static char *current_position;
static char *last_position;
static char *data;
static char *org_data;

static int *entry;
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
static struct identifier *sym_table;
static struct identifier *last_identifier = {0};

#define MAX_FUNCTIONS 256
struct function {
    int id;
    char name[25];
    int* entry;
};
struct function function_table[MAX_FUNCTIONS] = {0};
static int function_id = 0;

struct member {
    struct identifier *ident;
    struct member *next;
    int offset;
    int type;
} *members[MAX_MEMBERS];

/* AST Node types */
enum ASTNodeType {
    AST_NUM,
    AST_STR,
    AST_IDENT,
    AST_BINOP,
    AST_UNOP,
    AST_FUNCALL,
    AST_FUNCDEF,
    AST_VARDECL,
    AST_IF,
    AST_WHILE,
    AST_RETURN,
    AST_BLOCK,
    AST_ASSIGN,
    AST_EXPR_STMT,
    AST_SWITCH,
    AST_CASE,
    AST_DEFAULT,
    AST_BREAK,
    AST_MEMBER_ACCESS,
    AST_DEREF,
    AST_ADDR,
    AST_ENTER,
    AST_LEAVE,
    AST_ADJ
};

/* AST Node */
struct ASTNode {
    enum ASTNodeType type;
    int value;
    int data_type;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
    struct identifier ident;
    struct member *member;
};

static struct ASTNode *ast_root;

/* Prototypes */
struct ASTNode *parse();
static void next();
static struct ASTNode *expression(int level);
static struct ASTNode *statement();
static void include(char *file);

int free_ast(struct ASTNode *node) {
    if (!node) return 0;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
    return 0;
}

int dbgprintf(const char *fmt, ...){
#ifdef DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
    return 0;
}

void generate_x86(struct ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            printf("movl $%d, %%eax\n", node->value);
            return;
        case AST_STR:
            printf("movl $%d, %%eax\n", node->value - (int)org_data);
            return;
        case AST_IDENT:

            /* Optimization: use movl directly instead of leal and movl */
            if(node->ident.class == Loc && (node->ident.type <= INT || node->ident.type >= PTR)){
                printf("movl %d(%%ebp), %%eax\n", node->value > 0 ? node->value*4 : node->value);
                return;
            }

            if (node->ident.class == Loc) {
                printf("leal %d(%%ebp), %%eax\n", node->value > 0 ? node->value*4 : node->value);
            } else if (node->ident.class == Glo) {
                printf("movl $%d, %%eax\n", node->ident.val - (int)org_data);
            } else {
                printf("Unknown identifier class\n");
                exit(-1);
            }

            /* Load value if it's not a pointer type */
            if (node->ident.type <= INT || node->ident.type >= PTR) {
                printf("%s (%%eax), %%eax\n", (node->ident.type == CHAR) ? "movzb" : "movl");
            }
            return;
        case AST_BINOP:
            /**
             * First the address of the left operand is pushed onto the stack,
             * then the right operand is evaluated and the result is stored in %eax.
             * The left operand is then popped into %ebx and the operation is performed. 
             */
            generate_x86(node->left);
            printf("pushl %%eax\n");
            generate_x86(node->right);
            printf("popl %%ebx\n");
            switch (node->value) {
                case Add: printf("addl %%ebx, %%eax\n"); break;
                case Sub: printf("subl %%ebx, %%eax\n"); break;
                case Mul: printf("imull %%ebx, %%eax\n"); break;
                case Div: 
                    printf("movl $0, %%edx\n");
                    printf("idivl %%ebx\n"); 
                    break;
                case Eq: printf("cmpl %%ebx, %%eax\nsete %%al\nmovzb %%al, %%eax\n"); break;
                case Ne: printf("cmpl %%ebx, %%eax\nsetne %%al\nmovzb %%al, %%eax\n"); break;
                case Lt: printf("cmpl %%ebx, %%eax\nsetl %%al\nmovzb %%al, %%eax\n"); break;
                case Le: printf("cmpl %%ebx, %%eax\nsetle %%al\nmovzb %%al, %%eax\n"); break;
                case Gt: printf("cmpl %%ebx, %%eax\nsetg %%al\nmovzb %%al, %%eax\n"); break;
                case Ge: printf("cmpl %%ebx, %%eax\nsetge %%al\nmovzb %%al, %%eax\n"); break;
                default:
                    printf("Unknown binary operator %d\n", node->value);
                    exit(-1);
            }
            break;
        case AST_UNOP:
            generate_x86(node->left);
            switch (node->value) {
                case '!':
                    printf("cmpl $0, %%eax\n");
                    printf("sete %%al\n");
                    printf("movzb %%al, %%eax\n");
                    break;
                default:
                    printf("Unknown unary operator\n");
                    exit(-1);
            }
            break;
        case AST_FUNCALL:
            /* Collect arguments in a list to push them in reverse order */
            if (node->left) {
                struct ASTNode *args[16];
                int arg_count = 0;
                struct ASTNode *arg = node->left;
                while (arg) {
                    if (arg_count >= 16) {
                        printf("Too many arguments for function call\n");
                        exit(-1);
                    }
                    args[arg_count++] = arg;
                    arg = arg->next;
                }
                for (int i = arg_count - 1; i >= 0; i--) {
                    arg = args[i];
                    generate_x86(arg);
                    printf("pushl %%eax\n");
                }
            }

            if (node->ident.class == Sys) {
                printf("call _syscall_%d\n", node->ident.val);
            } else if (node->ident.class == Fun) {
                printf("call %.*s\n", node->ident.name_length, node->ident.name);
            } else {
                printf("Unknown function call\n");
                exit(-1);
            }
            if (node->left) {
                int arg_count = 0;
                struct ASTNode *arg = node->left;
                while (arg) {
                    arg_count++;
                    arg = arg->next;
                }
                if (arg_count > 0) {
                    printf("addl $%d, %%esp\n", arg_count * 4);
                }
            }
            break;
        case AST_RETURN:
            if (node->left) {
                generate_x86(node->left);
            }
            /* leave and ret is done by AST_LEAVE */
            break;
        case AST_IF: {
            generate_x86(node->left);
            printf("cmpl $0, %%eax\n");
            printf("je .Lfalse\n");
            generate_x86(node->right->left);
            if (node->right->right) {
                printf("jmp .Lend\n");
                printf(".Lfalse:\n");
                generate_x86(node->right->right);
                printf(".Lend:\n");
            } else {
                printf(".Lfalse:\n");
            }
            break;
        }
        case AST_WHILE: {
            printf(".Lstart:\n");
            generate_x86(node->left);
            printf("cmpl $0, %%eax\n");
            printf("je .Lend\n");
            generate_x86(node->right);
            printf("jmp .Lstart\n");
            printf(".Lend:\n");
            break;
        }
        case AST_BLOCK:
            generate_x86(node->left);
            return;
        case AST_EXPR_STMT:
            generate_x86(node->left);
            break;
        case AST_ASSIGN:
            if (node->left->type == AST_IDENT) {
                if (node->left->ident.class == Loc) {
                    printf("leal %d(%%ebp), %%eax\n", node->left->value);
                } else if (node->left->ident.class == Glo) {
                    printf("movl $%d, %%eax\n", node->left->ident.val - (int)org_data);
                }
                printf("pushl %%eax\n");
            } else if (node->left->type == AST_MEMBER_ACCESS) {
                generate_x86(node->left->left);
                printf("pushl %%eax\n");
                printf("movl $%d, %%eax\n", node->left->member->offset);
                printf("addl (%%esp), %%eax\n");
                printf("movl %%eax, (%%esp)\n");
            } else {
                printf("Left-hand side of assignment must be an identifier or member access\n");
                exit(-1);
            }
            generate_x86(node->right);
            printf("popl %%ebx\n");
            printf("movl %%eax, (%%ebx)\n");
            break;
        case AST_MEMBER_ACCESS:
            generate_x86(node->left);
            printf("movl %d(%%eax), %%eax\n", node->member->offset);
            break;
        case AST_DEREF:
            generate_x86(node->left);
            printf("movl (%%eax), %%eax\n");
            break;
        case AST_ADDR:
            generate_x86(node->left);
            break;
        case AST_ENTER:
            printf("%.*s:\n", node->ident.name_length, node->ident.name);
            printf("pushl %%ebp\n");
            printf("movl %%esp, %%ebp\n");
            printf("subl $%d, %%esp\n", node->value);
            break;
        case AST_LEAVE:
            printf("addl $%d, %%esp\n", node->value);
            printf("popl %%ebp\n");
            printf("ret\n\n");
            break;
        default:
            printf("Unknown AST node type: %d\n", node->type);
            exit(-1);
    }

    if (node->next) {
        generate_x86(node->next);
    }
}

int add_function(int id, char* name, int name_length, int* entry)
{
    if(function_id >= MAX_FUNCTIONS){
        printf("Maximum number of functions reached\n");
        exit(-1);
    }

    struct function *f = function_table + id;
    f->id = id;
    memcpy(f->name, name, name_length);
    
    /* Null-terminate the name */
    f->name[name_length] = '\0';

    f->entry = entry;

    return id;
}

struct function *find_function_name(char *name, int name_length)
{
    struct function *f = function_table;
    while (f->id) {
        if (strncmp(f->name, name, name_length) == 0) {
            return f;
        }
        f++;
    }
    return NULL;
}

struct function *find_function_id(int id){
    struct function *f = function_table;
    while (f) {
        if (f->id == id) {
            return f;
        }
        f++;
    }
    return NULL;
}

/**
 * @brief Generate bytecode from the AST
 * 
 * @param node The root AST node
 */
void generate_bytecode(struct ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            *++last_emitted = IMM;
            *++last_emitted = node->value;
            return;
        case AST_STR:
            *++last_emitted = IMD;
            *++last_emitted = node->value - (int)org_data;
            return;
        case AST_IDENT:
            if (node->ident.class == Loc) {
                /**
                 * Load a local variable,
                 * node->value is either positive (arguments)
                 * or negativ (stack allocated).
                 */

                *++last_emitted = LEA;
                *++last_emitted = node->value;
            } else if (node->ident.class == Glo) {
                /**
                 * Load a global variable,
                 * node->ident.val is the offset from the data segment.
                 */

                *++last_emitted = IMD;
                *++last_emitted = node->ident.val - (int)org_data;
            } else {
                printf("Unknown identifier class\n");
                exit(-1);
            }

            /* We load value if it's not a pointer type */
            if (node->ident.type <= INT || node->ident.type >= PTR) {
                *++last_emitted = (node->ident.type == CHAR) ? LC : LI;
            }
            return;
        case AST_BINOP:
            generate_bytecode(node->left);
            *++last_emitted = PSH;
            generate_bytecode(node->right);
            switch (node->value) {
                case Add: *++last_emitted = ADD; break;
                case Sub: *++last_emitted = SUB; break;
                case Mul: *++last_emitted = MUL; break;
                case Div: *++last_emitted = DIV; break;
                case Eq:  *++last_emitted = EQ;  break;
                case Ne:  *++last_emitted = NE;  break;
                case Lt:  *++last_emitted = LT;  break;
                case Le:  *++last_emitted = LE;  break;
                case Gt:  *++last_emitted = GT;  break;
                case Ge:  *++last_emitted = GE;  break;
                case And: *++last_emitted = AND; break;
                case Or:  *++last_emitted = OR;  break;
                case Xor: *++last_emitted = XOR; break;
                case Shl: *++last_emitted = SHL; break;
                case Shr: *++last_emitted = SHR; break;
                default:
                    printf("Unknown binary operator %d\n", node->value);
                    exit(-1);
            }
            break;
        case AST_UNOP:
            generate_bytecode(node->left);
            switch (node->value) {
                case Ne:
                    *++last_emitted = PSH;
                    *++last_emitted = IMM;
                    *++last_emitted = 0;
                    *++last_emitted = EQ;
                    break;
                default:
                    printf("Unknown unary operator\n");
                    exit(-1);
            }
            break;
        case AST_FUNCALL:
            // Collect arguments in a list to push them in reverse order
            if (node->left) {

                struct ASTNode *args[16]; // assuming a max of 16 args for simplicity
                int arg_count = 0;
                struct ASTNode *arg = node->left;
                while (arg) {
                    if (arg_count >= 16) {
                        printf("Too many arguments for function call\n");
                        exit(-1);
                    }
                    args[arg_count++] = arg;
                    arg = arg->next;
                }
                // Push arguments in reverse order
                for (int i = arg_count - 1; i >= 0; i--) {
                    arg = args[i];
                    if (arg->type == AST_MEMBER_ACCESS) {
                        generate_bytecode(arg->left); // Load base address
                        *++last_emitted = PSH; // Push base address
                        *++last_emitted = IMM;
                        *++last_emitted = arg->member->offset; // Add member offset
                        *++last_emitted = ADD;
                        *++last_emitted = (arg->data_type == CHAR) ? LC : LI; // Load value
                    } else {
                        generate_bytecode(arg);
                        
                    }
                    *++last_emitted = PSH;
                }
            }

            // Function call.
            if (node->ident.class == Sys) {
                *++last_emitted = node->ident.val;
            } else if (node->ident.class == Fun) {
                *++last_emitted = JSR;
                
                struct function *f = find_function_id(node->ident.val);
                if (!f || !f->entry) {
                    printf("Function %.*s not found in JSR\n", node->ident.name_length, node->ident.name);
                    exit(-1);
                }
                *++last_emitted = (int)f->entry - (int)emitted_code;

            } else {
                printf("Unknown function call\n");
                exit(-1);
            }
            if (node->left) {
                int arg_count = 0;
                struct ASTNode *arg = node->left;
                while (arg) {
                    arg_count++;
                    arg = arg->next;
                }
                *++last_emitted = ADJ;
                *++last_emitted = arg_count;
            }
            return;
        case AST_RETURN:
            if (node->left) {
                generate_bytecode(node->left);
            }
            *++last_emitted = LEV;
            break;
        case AST_IF: {
            generate_bytecode(node->left);
            *++last_emitted = BZ;
            int *false_jump = ++last_emitted;
            generate_bytecode(node->right->left);
            if (node->right->right) {
                *++last_emitted = JMP;
                int *end_jump = ++last_emitted;
                *false_jump = (int)(last_emitted + 1) - (int)emitted_code;
                generate_bytecode(node->right->right);
                *end_jump = (int)(last_emitted + 1) - (int)emitted_code;
            } else {
                *false_jump = (int)(last_emitted + 1) - (int)emitted_code;
            }
            break;
        }
        case AST_WHILE: {
            int *start = last_emitted + 1;
            generate_bytecode(node->left);
            *++last_emitted = BZ;
            int *end = ++last_emitted;
            generate_bytecode(node->right);
            *++last_emitted = JMP;
            *++last_emitted = (int)((int)start -(int)emitted_code);
            *end = (int)(last_emitted + 1) - (int)emitted_code;
            break;
        }
        case AST_BLOCK: {
            struct ASTNode *current = node->left;
            generate_bytecode(current);
            return;
        }
        case AST_EXPR_STMT:
            generate_bytecode(node->left);
            break;
        case AST_ASSIGN:
            if (node->left->type == AST_IDENT) {
                if (node->left->ident.class == Loc) {
                    *++last_emitted = LEA;
                    *++last_emitted = node->left->value;
                } else if (node->left->ident.class == Glo) {
                    *++last_emitted = IMD;
                    *++last_emitted = node->left->ident.val - (int)org_data;
                }
                *++last_emitted = PSH;
            } else if (node->left->type == AST_MEMBER_ACCESS) {
                generate_bytecode(node->left->left);
                *++last_emitted = PSH;
                *++last_emitted = IMM;
                *++last_emitted = node->left->member->offset;
                *++last_emitted = ADD;
                *++last_emitted = PSH;
            } else {
                printf("Left-hand side of assignment must be an identifier or member access\n");
                exit(-1);
            }
            generate_bytecode(node->right);
            *++last_emitted = (node->left->data_type == CHAR) ? SC : SI;
            break;
        case AST_MEMBER_ACCESS: {
            generate_bytecode(node->left); /* Load the address of the base identifier */
            *++last_emitted = PSH; 
            *++last_emitted = IMM; 
            *++last_emitted = node->member->offset;
            *++last_emitted = ADD;

            // Load value if it's not a pointer type
            if (node->data_type <= INT || node->data_type >= PTR) {
                *++last_emitted = (node->data_type == CHAR) ? LC : LI;
            }
            break;
        }
        case AST_DEREF:
            generate_bytecode(node->left);
            if (node->data_type <= INT || node->data_type >= PTR) {
                *++last_emitted = (node->data_type == CHAR) ? LC : LI;
            }
            break;
        case AST_ADDR:
            generate_bytecode(node->left);
            if (*last_emitted == LC || *last_emitted == LI) {
                --last_emitted;
            }
            break;
        case AST_ENTER:
            *++last_emitted = ENT;

            struct function *f = find_function_id(node->ident.val);
            if (!f) {
                printf("Function %.*s not found\n", node->ident.name_length, node->ident.name);
                exit(-1);
            }
            f->entry = last_emitted;
            entry = last_emitted;
            *++last_emitted = node->value;
            break;
        case AST_LEAVE:
            *++last_emitted = LEV;
            break;
        default:
            printf("Unknown AST node type: %d\n", node->type);
            exit(-1);
    }

    if (node->next) {
        generate_bytecode(node->next);
    }
}

static void include(char *file) {
    int fd, len;

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


                            printf("Including file: %s\n", include_file);

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
 * @brief Parse an expression and construct an AST node
 * A expression is a sequence of terms separated by + or - operators
 * @param level depth of the expression
 * @return struct ASTNode* - the root of the constructed AST for the expression
 */
static struct ASTNode *expression(int level) {
    struct ASTNode *node = NULL, *left, *right;
    int t; /* Temporary register */
    struct identifier *id;
    struct member *m;
    int sz;

    switch(token){
        case 0:
            printf("%d: unexpected token EOF of expression\n", line);
            exit(-1);
        case Num:
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_NUM;
            node->value = ival;
            node->data_type = INT;
            next();
            break;
        case '"':
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_STR;
            node->value = ival;
            next();
            while (token == '"') {
                next();
            }
            data = (char *)(((int)data + sizeof(int)) & -sizeof(int));
            node->data_type = PTR;
            break;
        case Sizeof:
            next();
            if(token == '('){
                next();
            }else {
                printf("%d: open parenthesis expected in sizeof\n", line);
                exit(-1);
            }

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_NUM;
            node->data_type = INT;
            node->value = 0;

            if(token == Int){
                node->value = sizeof(int);
                next();
            } else if(token == Char){
                node->value = sizeof(char);
                next();
            } else if (token == Struct){
                next();
                if(token != Id){
                    printf("%d: bad struct type\n", line);
                    exit(-1);
                }
                t = last_identifier->stype;
                node->value = type_size[t];
                next();
            } 

            while(token == Mul){
                next();
                node->value *= sizeof(int);
            }

            if(token == ')'){
                next();
            } else {
                printf("%d: close parenthesis expected in sizeof\n", line);
                exit(-1);
            }
            break;
        case Id:
            id = last_identifier;
            next();

            if(token == '('){
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_FUNCALL;
                node->ident = *id;
                node->left = NULL;
                node->right = NULL;

                next();
                if(token != ')'){
                    node->left = expression(Assign);
                    while(token == ','){
                        next();
                        struct ASTNode *arg = expression(Assign);
                        arg->next = node->left;
                        node->left = arg;
                    }
                }
                next();
                break;
            }

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_IDENT;
            node->ident = *id;
            node->data_type = id->type;

            if(id->class == Loc){
                node->value = local_offset - id->val;
                dbgprintf("%.*s> Local offset: %d (%d - %d)\n", node->ident.name_length, node->ident.name, node->value, local_offset, id->val);
            } else if(id->class == Glo){
                node->value = id->val - (int)org_data;
            } else {
                printf("%d: undefined variable\n", line);
                exit(-1);
            }
            break;
        case '(':
            next();
            node = expression(Assign);
            if(token == ')'){
                next();
            } else {
                printf("%d: bad expression\n", line);
                exit(-1);
            }
            break;
        
        case Mul:
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_DEREF;
            node->left = expression(Inc);
            if(node->left->data_type <= INT){
                printf("%d: bad dereference\n", line);
                exit(-1);
            }
            node->data_type = node->left->data_type - PTR;
            break; 
        case And:
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_ADDR;
            node->left = expression(Inc);
            node->data_type = node->left->data_type + PTR;
            break;
        
        case '!':
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_UNOP;
            node->left = expression(Inc);
            node->data_type = INT;
            node->value = Ne;
            break;

        case '~':
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_UNOP;
            node->left = expression(Inc);
            node->data_type = INT;
            node->value = Xor;
            break;

        case Add:
            next();
            node = expression(Inc);
            break;
        
        case Sub:
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_UNOP;
            node->left = expression(Inc);
            node->data_type = INT;
            node->value = Sub;
            break;

        case Inc:
        case Dec:
            t = token;
            next();
            node = expression(Inc);

            struct ASTNode *op_node = malloc(sizeof(struct ASTNode));
            op_node->type = AST_BINOP;
            op_node->left = node;
            op_node->right = malloc(sizeof(struct ASTNode));
            op_node->right->type = AST_NUM;
            op_node->right->value = (node->data_type >= PTR2 ? sizeof(int) : (node->data_type >= PTR) ? type_size[node->data_type - PTR] : 1);
            op_node->right->data_type = INT;
            op_node->value = (t == Inc) ? Add : Sub;

            struct ASTNode *assign_node = malloc(sizeof(struct ASTNode));
            assign_node->type = AST_ASSIGN;
            assign_node->left = node;
            assign_node->right = op_node;
            assign_node->data_type = node->data_type;
            node = assign_node;
            break;
        default:
            printf("%d: bad expression\n", line);
            exit(-1);
    }

    while(token >= level){
        left = node;

        switch(token){
            case Assign:
                next();
                right = expression(Assign);

                node = malloc(sizeof(struct ASTNode));
                node->type = AST_ASSIGN;
                node->left = left;
                node->right = right;
                node->data_type = left->data_type;
                break;
            case Cond:
                next();
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->value = Cond;
                node->right = malloc(sizeof(struct ASTNode));
                node->right->left = expression(Assign);
                if(token == ':'){
                    next();
                    node->right->right = expression(Cond);
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                break;
            case Lor:
                next();
                right = expression(Lan);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Lor;
                node->data_type = INT;
                break;
            case Lan:
                next();
                right = expression(Or);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Lan;
                node->data_type = INT;
                break;
            
            case Or:
                next();
                right = expression(Xor);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Or;
                node->data_type = INT;
                break;

            case Xor:
                next();
                right = expression(And);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Xor;
                node->data_type = INT;
                break;

            case And:
                next();
                right = expression(Eq);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = And;
                node->data_type = INT;
                break;
            
            case Eq:
                next();
                right = expression(Lt);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Eq;
                node->data_type = INT;
                break;
            
            case Ne:
                next();
                right = expression(Lt);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Ne;
                node->data_type = INT;
                break;
            
            case Lt:
                next();
                right = expression(Shl);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Lt;
                node->data_type = INT;
                break;
            
            case Gt:
                next();
                right = expression(Shl);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Gt;
                node->data_type = INT;
                break;
            
            case Le:
                next();
                right = expression(Shl);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Le;
                node->data_type = INT;
                break;
            
            case Ge:
                next();
                right = expression(Shl);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Ge;
                node->data_type = INT;
                break;
            
            case Shl:
                next();
                right = expression(Add);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Shl;
                node->data_type = INT;
                break;
            
            case Shr:
                next();
                right = expression(Add);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Shr;
                node->data_type = INT;
                break;
            
            case Add:
                next();
                right = expression(Mul);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Add;
                node->data_type = left->data_type;
                break;
            
            case Sub:
                next();
                right = expression(Mul);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Sub;
                node->data_type = left->data_type;
                break; 
            case Mul:
                next();
                right = expression(Inc);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Mul;
                node->data_type = INT;
                break;

            case Div:
                next();
                right = expression(Inc);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Div;
                node->data_type = INT;
                break;
            
            case Mod:
                next();
                right = expression(Inc);
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Mod;
                node->data_type = INT;
                break;
            
            case Inc:
            case Dec:
                t = token;
                next();

                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = malloc(sizeof(struct ASTNode));
                node->right->type = AST_NUM;
                node->right->value = (left->data_type >= PTR2 ? sizeof(int) : (left->data_type >= PTR) ? type_size[left->data_type - PTR] : 1);
                node->right->data_type = INT;
                node->value = (t == Inc) ? Add : Sub;
                node->data_type = left->data_type;

                struct ASTNode *assign_node = malloc(sizeof(struct ASTNode));
                assign_node->type = AST_ASSIGN;
                assign_node->left = left;
                assign_node->right = node;
                assign_node->data_type = left->data_type;
                node = assign_node;
                break;

            case Dot:
                left->data_type += PTR;
                // fall through to Arrow case

            case Arrow:
                if (left->data_type <= PTR + INT || left->data_type >= PTR2) {
                    printf("%d: illegal use of ->\n", line);
                    exit(-1);
                }
                next();
                if (token != Id) {
                    printf("%d: illegal use of ->\n", line);
                    exit(-1);
                }
                m = members[left->data_type - PTR];
                while (m && m->ident != last_identifier) {
                    m = m->next;
                }
                if (!m) {
                    printf("%d: struct member not found\n", line);
                    exit(-1);
                }

                struct ASTNode *binop_node = malloc(sizeof(struct ASTNode));
                binop_node->type = AST_BINOP;
                binop_node->left = left; // Left is the identifier

                binop_node->right = malloc(sizeof(struct ASTNode));
                binop_node->right->type = AST_NUM;
                binop_node->right->value = m->offset;
                binop_node->right->data_type = INT;

                binop_node->value = Add;
                binop_node->data_type = left->data_type;

                node = binop_node;
                    // Set these properties regardless of the offset condition
                node->type = AST_MEMBER_ACCESS;
                node->member = m;
                node->data_type = m->type;

                next();
                break;

            case Brak:
                next();
                right = expression(Assign);
                if(token == ']'){
                    next();
                } else {
                    printf("%d: close bracket expected\n", line);
                    exit(-1);
                }

                if (left->data_type < PTR){
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                sz = (left->data_type - PTR) >= PTR2 ? sizeof(int) : type_size[left->data_type - PTR];
                if(sz > 1){
                    node = malloc(sizeof(struct ASTNode));
                    node->type = AST_BINOP;
                    node->left = right;
                    node->right = malloc(sizeof(struct ASTNode));
                    node->right->type = AST_NUM;
                    node->right->value = sz;
                    node->right->data_type = INT;
                    node->value = Mul;
                    node->data_type = right->data_type;
                    right = node;
                }
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_BINOP;
                node->left = left;
                node->right = right;
                node->value = Add;
                node->data_type = left->data_type;
                break;
            default:
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
        }
    }

    return node;
}


static struct ASTNode *statement() {
    struct ASTNode *node, *condition, *body, *init, *increment;

    switch(token){
        case If:
            next();
            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();

            condition = expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_IF;
            node->left = condition;
            node->right = malloc(sizeof(struct ASTNode));
            node->right->left = statement();

            if(token == Else){
                next();
                node->right->right = statement();
            } else {
                node->right->right = NULL;
            }

            return node;
        
        case While:
            next();
            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();
            
            condition = expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_WHILE;
            node->left = condition;
            node->right = statement();

            return node;

        case Switch:
            next();
            if(token != '('){
                printf("%d: open parenthesis expected\n", line);
                exit(-1);
            }
            next();

            condition = expression(Assign);

            if(token != ')'){
                printf("%d: close parenthesis expected\n", line);
                exit(-1);
            }
            next();

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_SWITCH;
            node->left = condition;
            node->right = statement();

            return node;
        
        case Case:
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_CASE;
            node->left = expression(Or);
            if(token != ':'){
                printf("%d: colon expected\n", line);
                exit(-1);
            }
            next();
            node->right = statement();

            return node;
        
        case Break:
            next();
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_BREAK;

            return node;

        case Default:
            next();
            if(token != ':'){
                printf("%d: colon expected\n", line);
                exit(-1);
            }
            next();

            node = malloc(sizeof(struct ASTNode));
            node->type = AST_DEFAULT;
            node->left = statement();

            return node;

        case Return:
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_RETURN;
            if(token != ';'){
                node->left = expression(Assign);
            } else {
                node->left = NULL;
            }
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();

            return node;

        case '{':
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_BLOCK;
            node->left = statement();
            struct ASTNode *current = node->left;

            while(token != '}'){
                current->next = statement();
                current = current->next;
            }
            next();
            return node;
        
        case ';':
            next();
            node = malloc(sizeof(struct ASTNode));
            node->type = AST_EXPR_STMT;
            node->left = NULL;
            return node;
        
        default:
            node = expression(Assign);
            if(token != ';'){
                printf("%d: semicolon expected\n", line);
                exit(-1);
            }
            next();
            struct ASTNode *stmt = malloc(sizeof(struct ASTNode));
            stmt->type = AST_EXPR_STMT;
            stmt->left = node;
            return stmt;
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
struct ASTNode *root = NULL;
struct ASTNode *current = NULL;

struct ASTNode* parse() {
    int i;
    int bt;
    int ty;
    int mbt;
    struct member *m;
    struct identifier* func;


    while(token) {
        bt = INT;

        if(token == Int) next();
        else if(token == Char) {
            next();
            bt = CHAR;
        } else if(token == Enum) {
            next();
            if(token != '{') next();
            if(token == '{') {
                next();
                i = 0;
                while(token != '}') {
                    if(token != Id) {
                        printf("%d: bad enum identifier %d\n", line, token);
                        exit(-1);
                    }
                    next();
                    if(token == Assign) {
                        next();
                        if(token != Num) {
                            printf("%d: bad enum initializer\n", line);
                            exit(-1);
                        }
                        i = ival;
                        next();
                    }
                    last_identifier->class = Num;
                    last_identifier->type = INT;
                    last_identifier->val = i++;
                    if(token == ',') next();
                }
                next();
            }
        } else if( token == Struct ) {
            next();
            if(token == Id) {
                if(!last_identifier->stype) {
                    last_identifier->stype = type_new++;
                }
                bt = last_identifier->stype;
                next();
            } else {
                bt = type_new++;
            }

            if(token == '{') {
                next();
                if(members[bt]) {
                    printf("%d: duplicate struct definition\n", line);
                    exit(-1);
                }

                i = 0;

                while(token != '}') {
                    mbt = INT;
                    if(token == Int) next();
                    else if(token == Char) {
                        next();
                        mbt = CHAR;
                    } else if(token == Struct) {
                        next();
                        if(token != Id) {
                            printf("%d: bad struct member\n", line);
                            exit(-1);
                        }
                        mbt = last_identifier->stype;
                        next();
                    }

                    while(token != ';') {
                        ty = mbt;
                        while(token == Mul) {
                            next();
                            ty = ty + PTR;
                        }

                        if(token != Id) {
                            printf("%d: bad struct member\n", line);
                            exit(-1);
                        }

                        m = (struct member*) malloc(sizeof(struct member));
                        if(!m) {
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
                        if(token == ',') next();
                    }
                    next();
                }
                next();
                type_size[bt] = i;
            }
        }
        while(token != ';' && token != '}') {
            ty = bt;
            while(token == Mul) {
                next();
                ty = ty + PTR;
            }

            if(token != Id) {
                printf("%d: bad global declaration\n", line);
                exit(-1);
            }

            if(last_identifier->class) {
                printf("%d: duplicate global definition, %d\n", line, last_identifier->class);
                exit(-1);
            }

            next();

            last_identifier->type = ty;

            /* Check for function */
            if(token == '(') {
                last_identifier->class = Fun;
                last_identifier->val = function_id++;
                add_function(last_identifier->val, last_identifier->name, last_identifier->name_length, NULL);

                func = last_identifier;

                next();

                i = 0;
                while(token != ')') {
                    ty = INT;
                    if(token == Int) next();
                    else if(token == Char) {
                        next();
                        ty = CHAR;
                    } else if(token == Struct) {
                        next();
                        if(token != Id) {
                            printf("%d: bad struct type\n", line);
                            exit(-1);
                        }
                        ty = last_identifier->stype;
                        next();
                    }

                    while(token == Mul) {
                        next();
                        ty = ty + PTR;
                    }

                    if(token != Id) {
                        printf("%d: bad function definition\n", line);
                        exit(-1);
                    }

                    if(last_identifier->class == Loc) {
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

                    if(token == ',') next();
                }
                next();


                if(token != '{') {
                    printf("%d: bad function definition\n", line);
                    exit(-1);
                }
                
                local_offset = ++i;

                next();

                while(token == Int || token == Char || token == Struct) {
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
                        last_identifier->val = i + (ty >= PTR ? sizeof(int) : type_size[ty]);
                        i = last_identifier->val;

                        next();
                        if (token == ',') next();
                    }
                    next();
                }

                
                struct ASTNode *enter_node = malloc(sizeof(struct ASTNode));
                enter_node->type = AST_ENTER;
                enter_node->value = (i - local_offset);
                enter_node->ident = *func;
                enter_node->left = NULL;
                enter_node->right = NULL;
                enter_node->next = NULL;

                if (!root) {
                    root = enter_node;
                } else {
                    current->next = enter_node;
                }
                current = enter_node;

                while(token != '}') {
                    struct ASTNode *stmt = statement();
                    current->next = stmt;
                    current = stmt;
                }

                struct ASTNode *leave_node = malloc(sizeof(struct ASTNode));
                leave_node->type = AST_LEAVE;
                leave_node->value = i - local_offset;
                leave_node->ident = *func;
                leave_node->left = NULL;
                leave_node->right = NULL;
                leave_node->next = NULL;

                current->next = leave_node;
                current = leave_node;

                last_identifier = sym_table;
                while(last_identifier->tk) {
                    if(last_identifier->class == Loc) {
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
            if(token == ',') next();
        }
        next();
    }
    return root;
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
            case PRTF: {
                    t = sp + pc[1];
                    a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]);
                } 
                break;
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
    ast_root = parse();

    print_ast(ast_root);

    //generate_x86_asm(ast_root);

    generate_bytecode(ast_root);

    main_identifier->val = (int)entry;

    generate_x86(ast_root);

    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);

    int *pc;
    if (!(pc = (int *)main_identifier->val)) {
        printf("main() not defined\n");
        exit(-1);
    }

    printf("PC = %p %p\n", pc);

    
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

void print_ast_node(struct ASTNode *node, int indent_level) {
    if (!node) return;

    // Print the indentation
    for (int i = 0; i < indent_level; i++) {
        printf("  ");
    }

    // Print the current node
    switch (node->type) {
        case AST_NUM:
            printf("NUM: %d\n", node->value);
            break;
        case AST_STR:
            printf("STR: %d\n", node->value);
            break;
        case AST_IDENT:
            printf("IDENT: %.*s (%d)\n", node->ident.name_length, node->ident.name, node->value);
            break;
        case AST_BINOP:
            printf("BINOP: %d\n", node->value);
            break;
        case AST_UNOP:
            printf("UNOP: %d\n", node->value);
            break;
        case AST_FUNCALL:
            printf("FUNCALL: %.*s\n", node->ident.name_length, node->ident.name);
            break;
        case AST_FUNCDEF:
            printf("FUNCDEF\n");
            break;
        case AST_VARDECL:
            printf("VARDECL\n");
            break;
        case AST_IF:
            printf("IF\n");
            break;
        case AST_WHILE:
            printf("WHILE\n");
            break;
        case AST_RETURN:
            printf("RETURN\n");
            break;
        case AST_BLOCK:
            printf("BLOCK\n");
            break;
        case AST_ASSIGN:
            printf("ASSIGN\n");
            break;
        case AST_EXPR_STMT:
            printf("EXPR_STMT\n");
            break;
        case AST_SWITCH:
            printf("SWITCH\n");
            break;
        case AST_CASE:
            printf("CASE\n");
            break;
        case AST_DEFAULT:
            printf("DEFAULT\n");
            break;
        case AST_BREAK:
            printf("BREAK\n");
            break;
        case AST_MEMBER_ACCESS:
            printf("MEMBER_ACCESS: %.*s\n", node->member->ident->name_length, node->member->ident->name);
            break;
        case AST_DEREF:
            printf("DEREF\n");
            break;
        case AST_ADDR:
            printf("ADDR\n");
            break;
        case AST_ENTER:
            printf("ENTER 0x%x (%.*s)\n", node->value, node->ident.name_length, node->ident.name);
            break;
        case AST_LEAVE:
            printf("LEAVE\n");
            break;
        default:
            printf("UNKNOWN NODE TYPE\n");
            break;
    }


    if (node->left) {
        print_ast_node(node->left, indent_level + 1);
    }
    if (node->right) {
        print_ast_node(node->right, indent_level + 1);
    }
    if (node->next) {
        print_ast_node(node->next, indent_level);
    }
}

void print_ast(struct ASTNode *root) {
    print_ast_node(root, 0);
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

    free_ast(ast_root);

    return 0;
}
