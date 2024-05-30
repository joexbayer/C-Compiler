
#include <ast.h>
#include <func.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>  /* Include for chmod */

/* TODO: Change this... */
static int* data_section = (int*)0x08048000;

static uint8_t opcodes[1024] = {0};
static int opcodes_count = 0;

int asmprintf(FILE* file, const char *format, ...) {
    va_list args;
    va_start(args, format);

    if(file){
        vfprintf(file, format, args);
    }

    va_end(args);
    return 0;
}

#define ADJUST_SIZE(node) (node->value > 0 ? node->value*4 : node->value)

#define GEN_X86_LEAL_EBP(val)\
    opcodes[opcodes_count++] = 0x8d;\
    opcodes[opcodes_count++] = 0x45;\
    *((int*)(opcodes + opcodes_count)) = val;\
    opcodes_count += 4;

#define GEN_X86_ESP_EBP()\
    opcodes[opcodes_count++] = 0x89;\
    opcodes[opcodes_count++] = 0xe5;

#define GEN_X86_PUSH_EBP()\
    opcodes[opcodes_count++] = 0x55;

#define GEN_X86_SUB_ESP(val)\
    opcodes[opcodes_count++] = 0x81;\
    opcodes[opcodes_count++] = 0xec;\
    *((int*)(opcodes + opcodes_count)) = val;\
    opcodes_count += 4;

#define GEN_X86_ADD_ESP(val)\
    opcodes[opcodes_count++] = 0x81;\
    opcodes[opcodes_count++] = 0xc4;\
    *((int*)(opcodes + opcodes_count)) = val;\
    opcodes_count += 4;

#define GEN_X86_POP_EBP()\
    opcodes[opcodes_count++] = 0x5d;

#define GEN_X86_POP_EBX()\
    opcodes[opcodes_count++] = 0x5b;

#define GEN_X86_PUSH_EAX()\
    opcodes[opcodes_count++] = 0x50;

#define GEN_X86_RET()\
    opcodes[opcodes_count++] = 0xc3;

#define GEN_X86_CALL(offset)\
    opcodes[opcodes_count++] = 0xe8;\
    *((int*)(opcodes + opcodes_count)) = offset;\
    opcodes_count += 4;

#define GEN_X86_JMP(offset)\
    opcodes[opcodes_count++] = 0xe9;\
    *((int*)(opcodes + opcodes_count)) = offset;\
    opcodes_count += 4;

#define GEN_X86_EAX_EBX()\
    opcodes[opcodes_count++] = 0x89;\
    opcodes[opcodes_count++] = 0xc3;

#define GEN_X86_IMD_EAX(val)\
    opcodes[opcodes_count++] = 0xb8;\
    *((int*)(opcodes + opcodes_count)) = val;\
    opcodes_count += 4;

#define GEN_X86_INT(val)\
    opcodes[opcodes_count++] = 0xcd;\
    opcodes[opcodes_count++] = val;

int derefence = 0;
int lable_count = 0;
void generate_x86(struct ASTNode *node, FILE *file) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            asmprintf(file, "movl $%d, %%eax\n", node->value);
            GEN_X86_IMD_EAX(node->value);
            return;
        case AST_STR:
            asmprintf(file, "movl $%d, %%eax\n", node->value - (int)data_section);

            printf("TBD: Implement string\n");
            exit(-1);
            return;
        case AST_IDENT:
            /* Optimization: use movl directly instead of leal and movl */
            if(node->ident.class == Loc && (node->ident.type <= INT || node->ident.type >= PTR)){
                asmprintf(file, "movl %d(%%ebp), %%eax\n", node->value > 0 ? node->value*4 : node->value);

                opcodes[opcodes_count++] = 0x8b;
                opcodes[opcodes_count++] = 0x45;
                opcodes[opcodes_count++] = ADJUST_SIZE(node);
                return;
            }

            if (node->ident.class == Loc) {
                asmprintf(file, "leal %d(%%ebp), %%eax\n", node->value > 0 ? node->value*4 : node->value);
                
                opcodes[opcodes_count++] = 0x8d;
                opcodes[opcodes_count++] = 0x45;
                opcodes[opcodes_count++] = node->value;
                
            } else if (node->ident.class == Glo) {
                asmprintf(file, "movl $%d, %%eax\n", node->ident.val - (int)data_section);

                
                printf("TBD: Implement global variable\n");
                exit(-1);
            } else {
                asmprintf(file, "Unknown identifier class\n");
                exit(-1);
            }

            /* Load value if it's not a pointer type */
            if (node->ident.type <= INT || node->ident.type >= PTR) {
                asmprintf(file, "%s (%%eax), %%eax\n", (node->ident.type == CHAR) ? "movzb" : "movl");

                opcodes[opcodes_count++] = 0x0f;
                opcodes[opcodes_count++] = 0xb6;
                opcodes[opcodes_count++] = 0x00;
            }
            return;
        case AST_BINOP:
            /**
             * First the address of the left operand is pushed onto the stack,
             * then the right operand is evaluated and the result is stored in %eax.
             * The left operand is then popped into %ebx and the operation is performed. 
             */
            generate_x86(node->left, file);

            asmprintf(file, "pushl %%eax\n");
            opcodes[opcodes_count++] = 0x50;

            generate_x86(node->right, file);

            asmprintf(file, "popl %%ebx\n");
            opcodes[opcodes_count++] = 0x5b;

            switch (node->value) {
                case Add: {
                    asmprintf(file, "addl %%ebx, %%eax\n");
                    opcodes[opcodes_count++] = 0x01;
                    opcodes[opcodes_count++] = 0xd8;
                    }break;
                case Sub: {
                    asmprintf(file, "subl %%ebx, %%eax\n");
                    opcodes[opcodes_count++] = 0x29;
                    opcodes[opcodes_count++] = 0xd8;

                    } break;
                case Mul:{
                    asmprintf(file, "imull %%ebx, %%eax\n"); 
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xaf;
                    opcodes[opcodes_count++] = 0xc3;
                    }break;
                case Div: {
                    asmprintf(file, "movl $0, %%edx\n");
                    asmprintf(file, "idivl %%ebx\n"); 
                    
                    opcodes[opcodes_count++] = 0x89;
                    opcodes[opcodes_count++] = 0xd2;
                    opcodes[opcodes_count++] = 0xf7;
                    opcodes[opcodes_count++] = 0xfb;
                    
                    }break;
                case Eq: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsete %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x94;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                case Ne: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsetne %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x95;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                case Lt: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsetg %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x9f;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                case Le: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsetge %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x9d;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                case Gt: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsetl %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x9c;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                case Ge: {
                    asmprintf(file, "cmpl %%ebx, %%eax\nsetle %%al\nmovzb %%al, %%eax\n");
                    opcodes[opcodes_count++] = 0x39;
                    opcodes[opcodes_count++] = 0xd8;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0x9e;
                    opcodes[opcodes_count++] = 0xc0;
                    opcodes[opcodes_count++] = 0x0f;
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;
                    } break;
                default:
                    printf("Unknown binary operator %d\n", node->value);
                    exit(-1);
            }
            break;
        case AST_UNOP:
            generate_x86(node->left, file);
            switch (node->value) {
                case Ne:
                    asmprintf(file, "cmpl $0, %%eax\n");
                    asmprintf(file, "sete %%al\n");
                    asmprintf(file, "movzb %%al, %%eax\n");

                    opcodes[opcodes_count++] = 0x83;
                    opcodes[opcodes_count++] = 0xf8;
                    
                    opcodes[opcodes_count++] = 0x00;
                    opcodes[opcodes_count++] = 0x0f;
                    
                    opcodes[opcodes_count++] = 0xb6;
                    opcodes[opcodes_count++] = 0xc0;

                    break;
                default:
                    printf("Unknown unary operator\n");
                    exit(-1);
            }
            break;
        case AST_FUNCALL:
            /* Collect arguments in a list to push them in reverse order */
            asmprintf(file, "# Function call\n");
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
                /* Push arguments in reverse order */
                for (int i = arg_count - 1; i >= 0; i--) {
                    arg = args[i];
                    generate_x86(arg, file);
                    asmprintf(file, "pushl %%eax\n");

                    opcodes[opcodes_count++] = 0x50;
                }
            }

            /* Sys functions are akin to __builtins */
            if(node->ident.class == Sys) {
                switch (node->ident.val) {
                    case INTERRUPT: {
                        /**
                         * Interrupt: For the time being, linux style interrupts.
                         * __interrupt(int number, int eax, int ebx, int ecx, int edx, int esi)
                         * The number is the interrupt to call.
                         * Use pushed arguments, pop them into registers and call interrupt.
                         */                        
                        /* Pop intterupt */
                        asmprintf(file, "popl %%esi\n"); opcodes[opcodes_count++] = 0x5e;
                        asmprintf(file, "popl %%edx\n"); opcodes[opcodes_count++] = 0x5a;
                        asmprintf(file, "popl %%ecx\n"); opcodes[opcodes_count++] = 0x59;
                        asmprintf(file, "popl %%ebx\n"); opcodes[opcodes_count++] = 0x5b;
                        asmprintf(file, "popl %%eax\n"); opcodes[opcodes_count++] = 0x58;

                        /* pop number */
                        asmprintf(file, "popl %%edi\n"); opcodes[opcodes_count++] = 0x5f;

                        /* xor edi and ebp */
                        asmprintf(file, "pushl %%ebp\n"); opcodes[opcodes_count++] = 0x55;
                        asmprintf(file, "xorl %%ebp, %%ebp\n"); opcodes[opcodes_count++] = 0x31; opcodes[opcodes_count++] = 0xed;
                        asmprintf(file, "xorl %%edi, %%edi\n"); opcodes[opcodes_count++] = 0x31; opcodes[opcodes_count++] = 0xff;

                        /* TODO: This is techincally only for mmap */
                        asmprintf(file, "dec %%edi\n"); opcodes[opcodes_count++] = 0x4f;

                        int interrupt = 0;
                        // last argument
                        struct ASTNode *arg = node->left;
                        while (arg->next) {
                            arg = arg->next;
                        }
                        interrupt = arg->value;

                        /* Call interrupt */
                        asmprintf(file, "int $0%d\n", interrupt);
                        GEN_X86_INT(interrupt);

                        asmprintf(file, "popl %%ebp\n"); opcodes[opcodes_count++] = 0x5d;

                        break;
                    }
                    default: {
                        printf("Unsupported builtin %d\n", node->ident.val);
                        exit(-1);
                    }
                }
            } else if (node->ident.class == Fun) {

                struct function *f = find_function_id(node->ident.val);
                if (!f || !f->entry) {
                    printf("Function %.*s not found in JSR\n", node->ident.name_length, node->ident.name);
                    exit(-1);
                }

                asmprintf(file, "call %.*s %d\n", node->ident.name_length, node->ident.name,-opcodes_count);
        
                int offset = (int)f->entry - opcodes_count;
                GEN_X86_CALL(offset-5);
            } else {
                printf("Unknown x86 function call\n");
                exit(-1);
            }
            if (node->left) {
                int arg_count = 0;
                struct ASTNode *arg = node->left;
                while (arg) {
                    arg_count++;
                    arg = arg->next;
                }
                if (arg_count > 0 && node->ident.class == Fun) {
                    asmprintf(file, "addl $%d, %%esp # Cleanup stack pushed arguments\n", arg_count * 4);

                    opcodes[opcodes_count++] = 0x81;
                    opcodes[opcodes_count++] = 0xc4;
                    *((int*)(opcodes + opcodes_count)) = arg_count * 4;
                    opcodes_count += 4;

                }
            }
            break;
        case AST_RETURN:
            if (node->left) {
                generate_x86(node->left, file);
            }
            /* leave and ret is done by AST_LEAVE */
            break;
        case AST_IF: {
            asmprintf(file, "# If statement\n");
            int lfalse = lable_count++;
            int lend = lable_count++;

            generate_x86(node->left, file);
            asmprintf(file, "cmpl $0, %%eax\n");
            asmprintf(file, "je .Lfalse%d\n", lfalse);
            asmprintf(file, "# If true\n");
            generate_x86(node->right->left, file);
            if (node->right->right) {
                asmprintf(file, "jmp .Lend%d\n", lend);
                asmprintf(file, ".Lfalse%d:\n", lfalse);
                generate_x86(node->right->right, file);
                asmprintf(file, ".Lend%d:\n", lend);
            } else {
                asmprintf(file, ".Lfalse%d:\n", lfalse);
            }
            break;
        }
        case AST_WHILE: {
            asmprintf(file, "# While statement\n");
            int lstart = lable_count++;
            int lend = lable_count++;


            asmprintf(file, ".Lstart%d:\n", lstart);
            int while_start = opcodes_count;

            generate_x86(node->left, file);

            asmprintf(file, "cmpl $0, %%eax\n");
            opcodes[opcodes_count++] = 0x83;
            opcodes[opcodes_count++] = 0xf8;
            opcodes[opcodes_count++] = 0x00;


            asmprintf(file, "je .Lend%d\n", lend);
            opcodes[opcodes_count++] = 0x0f;
            opcodes[opcodes_count++] = 0x84;
            int while_end_patch = opcodes_count;
            *((int*)(opcodes + opcodes_count)) = 0;
            opcodes_count += 4;
            
            /* Generate */
            generate_x86(node->right, file);

            asmprintf(file, "jmp .Lstart%d\n", lstart);
            opcodes[opcodes_count++] = 0xe9;
            *((int*)(opcodes + opcodes_count)) = while_start - opcodes_count - 4;
            opcodes_count += 4;

            asmprintf(file, ".Lend%d:\n", lend);
            *((int*)(opcodes + while_end_patch)) = opcodes_count - while_end_patch - 4;        

            break;
        }
        case AST_BLOCK:
            generate_x86(node->left, file);
            return;
        case AST_EXPR_STMT:
            generate_x86(node->left, file);
            break;
        case AST_ASSIGN:

            if(node->left->type != AST_IDENT && node->left->type != AST_MEMBER_ACCESS && node->left->type != AST_DEREF && node->left->type != AST_ADDR){
                printf("Left-hand side of assignment must be an identifier or member access\n");
                exit(-1);
            }
            if(node->right->type == AST_FUNCALL){
                /** 
                 * Optimization: If we know that right is a function, we know the result will be in %eax.
                 * Therefor we can simple store the result in the left operand.
                 */
                generate_x86(node->right, file);
                if(node->left->type == AST_IDENT){
                    if(node->left->ident.class == Loc){
                        asmprintf(file, "movl %%eax, %d(%%ebp)\n", node->left->value);
                        opcodes[opcodes_count++] = 0x89; opcodes[opcodes_count++] = 0x45; opcodes[opcodes_count++] = node->left->value;
                    }
                    else if(node->left->ident.class == Glo){
                        asmprintf(file, "movl %%eax, %d(%%data)\n", node->left->ident.val - (int)data_section);
                        opcodes[opcodes_count++] = 0xc7; opcodes[opcodes_count++] = 0x05; *((int*)(opcodes + opcodes_count)) = node->left->ident.val - (int)data_section; opcodes_count += 4;
                    }
                } else if(node->left->type == AST_MEMBER_ACCESS){
                    if(node->left->left->ident.class == Loc){
                        asmprintf(file, "movl %%eax, %d(%%ebp)\n", ADJUST_SIZE(node->left->left) + node->left->member->offset);
                        opcodes[opcodes_count++] = 0x89; opcodes[opcodes_count++] = 0x45; opcodes[opcodes_count++] = ADJUST_SIZE(node->left->left) + node->left->member->offset;
                    }
                    else if(node->left->left->ident.class == Glo){
                        asmprintf(file, "movl %%eax, %d(%%data)\n", node->left->left->ident.val - (int)data_section);
                        opcodes[opcodes_count++] = 0xc7; opcodes[opcodes_count++] = 0x05; *((int*)(opcodes + opcodes_count)) = node->left->left->ident.val - (int)data_section; opcodes_count += 4;
                    }
                    else {
                        printf("Unknown identifier class\n");
                        exit(-1);
                    }
                }
                return;
            }

            if(node->right->type == AST_NUM){
                /* Optimization: assign constant to variable */
                if (node->left->type == AST_IDENT) {
                    if (node->left->ident.class == Loc) {
                        asmprintf(file, "movl $%d, %d(%%ebp)\n", node->right->value, node->left->value);

                        opcodes[opcodes_count++] = 0xc7;
                        opcodes[opcodes_count++] = 0x45;
                        opcodes[opcodes_count++] = node->left->value;
                        *((int*)(opcodes + opcodes_count)) = node->right->value;
                        opcodes_count += 4;

                    } else if (node->left->ident.class == Glo) {
                        asmprintf(file, "movl $%d, %d(%%data)\n", node->right->value, node->left->ident.val - (int)data_section);
                        printf("TBD: Implement global variable\n");
                        exit(-1);
                    }
                } else if (node->left->type == AST_MEMBER_ACCESS) {

                    /* If the ident if a pointer, we need to adjust the code */
                    if(node->left->left->ident.type >= PTR && node->left->left->ident.type < PTR2){
                        asmprintf(file, "movl %d(%%ebp), %%eax\n", ADJUST_SIZE(node->left->left));
                        opcodes[opcodes_count++] = 0x8b;
                        opcodes[opcodes_count++] = 0x45;
                        opcodes[opcodes_count++] = ADJUST_SIZE(node->left->left);

                        asmprintf(file, "movl $%d, %d(%%eax)\n", node->right->value,  node->left->member->offset);
                        opcodes[opcodes_count++] = 0xc7;
                        opcodes[opcodes_count++] = 0x40;
                        opcodes[opcodes_count++] = node->left->member->offset;
                        *((int*)(opcodes + opcodes_count)) = node->right->value;
                        opcodes_count += 4;

                        return;
                    } 

                    if(node->left->left->ident.class == Loc){
                        asmprintf(file, "movl $%d, %d(%%ebp)\n", node->right->value, ADJUST_SIZE(node->left->left) + node->left->member->offset);
                        opcodes[opcodes_count++] = 0xc7;
                        opcodes[opcodes_count++] = 0x45;
                        opcodes[opcodes_count++] = ADJUST_SIZE(node->left->left) + node->left->member->offset;
                        *((int*)(opcodes + opcodes_count)) = node->right->value;
                        opcodes_count += 4;
                    }
                    else if(node->left->left->ident.class == Glo)
                        asmprintf(file, "movl $%d, %d(%%data)\n", node->right->value, node->left->left->ident.val - (int)data_section);               
                    else {
                        printf("Unknown identifier class\n");
                        exit(-1);
                    }
                } else if(node->left->type == AST_DEREF){
                    generate_x86(node->left->left, file);

                    asmprintf(file, "movl $%d, (%%eax)\n", node->right->value);
                    opcodes[opcodes_count++] = 0xc7;
                    opcodes[opcodes_count++] = 0x00;
                    *((int*)(opcodes + opcodes_count)) = node->right->value;
                    opcodes_count += 4;
                }
                return; 
            }

            /* Push the address of the left operand onto the stack */
            if (node->left->type == AST_IDENT) {
                
                /* If the ident if a pointer, we need to adjust the code */
                if(node->left->ident.type >= PTR && node->left->ident.type < PTR2 && node->right->type == AST_NUM){
                    asmprintf(file, "movl %d(%%ebp), %%eax\n", ADJUST_SIZE(node->left));
                    opcodes[opcodes_count++] = 0x8b;
                    opcodes[opcodes_count++] = 0x45;
                    opcodes[opcodes_count++] = ADJUST_SIZE(node->left);

                    asmprintf(file, "pushl %%eax\n");
                    opcodes[opcodes_count++] = 0x50;

                    asmprintf(file, "movl $%d, (%%eax)\n", node->right->value);
                    opcodes[opcodes_count++] = 0xc7;
                    opcodes[opcodes_count++] = 0x00;
                    *((int*)(opcodes + opcodes_count)) = node->right->value;
                    opcodes_count += 4;

                    return;
                }

                if (node->left->ident.class == Loc) {
                    asmprintf(file, "leal %d(%%ebp), %%eax\n", node->left->value);
                    opcodes[opcodes_count++] = 0x8d;
                    opcodes[opcodes_count++] = 0x45;
                    opcodes[opcodes_count++] = node->left->value;


                } else if (node->left->ident.class == Glo) {
                    asmprintf(file, "movl $%d, %%eax\n", node->left->ident.val - (int)data_section);
                }
                asmprintf(file, "pushl %%eax\n");
                opcodes[opcodes_count++] = 0x50;
            } else if (node->left->type == AST_MEMBER_ACCESS) {
                /* If the ident if a pointer, we need to adjust the code */
                if(node->left->left->ident.type >= PTR && node->left->left->ident.type < PTR2){
                    asmprintf(file, "movl %d(%%ebp), %%eax\n", ADJUST_SIZE(node->left->left));
                    opcodes[opcodes_count++] = 0x8b;
                    opcodes[opcodes_count++] = 0x45;
                    opcodes[opcodes_count++] = ADJUST_SIZE(node->left->left);

                    
                    if(node->right->type == AST_NUM){
                        asmprintf(file, "movl $%d, %d(%%eax)\n", node->right->value,  node->left->member->offset);
                        opcodes[opcodes_count++] = 0xc7;
                        opcodes[opcodes_count++] = 0x40;
                        opcodes[opcodes_count++] = node->left->member->offset;
                        *((int*)(opcodes + opcodes_count)) = node->right->value;
                        opcodes_count += 4;

                        return;
                    } else {
                        asmprintf(file, "pushl %%eax\n");
                        opcodes[opcodes_count++] = 0x50;

                        generate_x86(node->right, file);
                        asmprintf(file, "popl %%ebx\n");
                        opcodes[opcodes_count++] = 0x5b;
                        asmprintf(file, "movl %%eax, %d(%%ebx)\n", node->left->member->offset);
                        opcodes[opcodes_count++] = 0x89;
                        opcodes[opcodes_count++] = 0x43;
                        opcodes[opcodes_count++] = node->left->member->offset;
                
                    }
                    return;
                } 

                asmprintf(file, "leal %d(%%ebp), %%eax\n", ADJUST_SIZE(node->left->left) + node->left->member->offset );
                opcodes[opcodes_count++] = 0x8d;
                opcodes[opcodes_count++] = 0x45;
                opcodes[opcodes_count++] = ADJUST_SIZE(node->left->left) + node->left->member->offset;
                
                asmprintf(file, "pushl %%eax\n");
                opcodes[opcodes_count++] = 0x50;
            } else if(node->left->type == AST_DEREF){
                generate_x86(node->left->left, file);
                asmprintf(file, "pushl %%eax\n");
                opcodes[opcodes_count++] = 0x50;
            } else if(node->left->type == AST_ADDR){
                generate_x86(node->left->left, file);
                asmprintf(file, "pushl %%eax\n");
                opcodes[opcodes_count++] = 0x50;
            } else {
                printf("Left-hand side of assignment must be an identifier or member access\n");
                exit(-1);
            }

            asmprintf(file, "# Assignment\n");
            /* Evaluate the right operand and store the result in %eax */
            generate_x86(node->right, file);

            /* Pop the address of the left operand into %ebx and perform the assignment */
            asmprintf(file, "popl %%ebx\n");
            GEN_X86_POP_EBX();

            asmprintf(file, "movl %%eax, (%%ebx)\n");
            opcodes[opcodes_count++] = 0x89;
            opcodes[opcodes_count++] = 0x03;
            break;
        case AST_MEMBER_ACCESS:
            generate_x86(node->left, file);
            asmprintf(file, "movl %d(%%eax), %%eax\n", node->member->offset);
            opcodes[opcodes_count++] = 0x8b;
            opcodes[opcodes_count++] = 0x40;
            opcodes[opcodes_count++] = node->member->offset;

            return;
        case AST_DEREF:
            asmprintf(file, "# Dereference\n");
            generate_x86(node->left, file);
            asmprintf(file, "movl (%%eax), %%eax\n");
            opcodes[opcodes_count++] = 0x8b;
            opcodes[opcodes_count++] = 0x00;
            break;
        case AST_ADDR:
            asmprintf(file, "# Reference\n");
            asmprintf(file, "leal %d(%%ebp), %%eax\n", node->left->value);
            opcodes[opcodes_count++] = 0x8d;
            opcodes[opcodes_count++] = 0x45;
            opcodes[opcodes_count++] = node->left->value;
        
            
            //generate_x86(node->left, file);
            return;
        case AST_ENTER:
            asmprintf(file, "%.*s:\n", node->ident.name_length, node->ident.name);
            asmprintf(file, "# Setting up stack frame %d\n", opcodes_count);
            asmprintf(file, "pushl %%ebp\n");
            asmprintf(file, "movl %%esp, %%ebp\n");

            struct function *f = find_function_id(node->ident.val);
            if (!f) {
                printf("Function %.*s not found\n", node->ident.name_length, node->ident.name);
                exit(-1);
            }
            f->entry = (int*)opcodes_count;

            GEN_X86_PUSH_EBP();
            GEN_X86_ESP_EBP();

            if(node->value > 0){
                asmprintf(file, "subl $%d, %%esp\n", node->value);
                GEN_X86_SUB_ESP(node->value);
            }
            break;
        case AST_LEAVE:
            asmprintf(file, "# Cleaning up stack frame\n");
            if(node->value > 0){
                asmprintf(file, "addl $%d, %%esp\n", node->value);
                GEN_X86_ADD_ESP(node->value);
            }
            asmprintf(file, "popl %%ebp\n");
            asmprintf(file, "ret\n\n");

            GEN_X86_POP_EBP();
            GEN_X86_RET();
            break;
        default:
            printf("Unknown AST node type: %d\n", node->type);
            exit(-1);
    }

    if (node->next) {
        generate_x86(node->next, file);
    }
}

void write_opcodes(){
    FILE *file = fopen(config.output, "wb");

    if(config.elf)
        write_elf_header(file, config.org, opcodes_count, 0);

    fwrite(opcodes, sizeof(uint8_t), opcodes_count, file);
    fclose(file);

    chmod(config.output, 0777);
}


void write_x86(struct ASTNode *node, FILE *file) {
    /* Prepare jump to _start */
    opcodes[opcodes_count++] = 0xe9;
    uint8_t *placeholder = opcodes + opcodes_count;
    *((int*)(opcodes + opcodes_count)) = 0;
    opcodes_count += 4;
    
    generate_x86(node, file);

    asmprintf(file, ".globl _start\n");
    asmprintf(file, "_start:\n");
    asmprintf(file, "call main\n");
    /* Call absoulute address */

    struct function *f = find_function_name("main", 4);
    if (!f) {
        printf("Main function not found\n");
        exit(-1);
    }
    int offset = (int)f->entry - opcodes_count;
    
    /* Placeholder for jump to _start */
    *((int*)placeholder) = (opcodes_count-5);

    /* Should call main, not first function */
    GEN_X86_CALL((offset-5));

    asmprintf(file, "movl %%eax, %%ebx\n");
    GEN_X86_EAX_EBX();

    asmprintf(file, "movl $1, %%eax\n");
    GEN_X86_IMD_EAX(1);

    asmprintf(file, "int $0x80\n");
    GEN_X86_INT(0x80);

    write_opcodes();
}
