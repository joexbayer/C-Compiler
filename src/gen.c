#include <cc.h>
#include <func.h>
#include <ast.h>

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
                printf("Unknown function call> %d\n", node->ident.class);
                exit(-1);
            }
            if (node->left && node->ident.class == Fun) {
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
            } else if (node->left->type == AST_DEREF) {
                generate_bytecode(node->left->left);
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
            return;
        case AST_ADDR:
            generate_bytecode(node->left);
            if (*last_emitted == LC || *last_emitted == LI) {
                --last_emitted;
            }
            return;
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
