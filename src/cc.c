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

#include <ast.h>
#include <cc.h>
#include <func.h>

void generate_bytecode(struct ASTNode *node);
void generate_x86(struct ASTNode *node, FILE *file);
void print_ast(struct ASTNode *root);
void write_x86(struct ASTNode *node, FILE *file);
void run_virtual_machine(int *pc, int* code, char *data, int argc, char *argv[]);



#define DEBUG
#undef DEBUG

#define IS_LETTER(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '_')
#define IS_DIGIT(x) (x >= '0' && x <= '9')
#define IS_HEX_DIGIT(x) (IS_DIGIT(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'))

static char *current_position;
static char *last_position;

char *data;
char *org_data;

int *entry;
int *emitted_code;
int *last_emitted;

static int* type_size;
static int type_new = 0;

static int token;
static int ival;
static int type;
static int local_offset;

static int line;


/**
 * @brief Pointers used to store the current position in the file
 * when another file is included. Having these as global variables
 * limits us to a single "depth" of file inclusion.
 */
static char *original_position;
static char *original_last_position;
static int original_line;
static char *include_buffer;


static struct identifier *sym_table;
static struct identifier *last_identifier = {0};
static struct member *members[MAX_MEMBERS] = {0};

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

            } else if(id->class == Num){
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_NUM;
                node->value = id->val;
                node->data_type = id->type;
                type = id->type;
            } else {
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
                    printf("%d: undefined variable: class %d\n", line, id->class);
                    exit(-1);
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
                node = expression(Inc);
                node->data_type = t;
            } else {
                node = expression(Assign);
                if(token == ')'){
                    next();
                } else {
                    printf("%d: bad expression\n", line);
                    exit(-1);
                }
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
                left = node;  // The left node is now the address calculation result

                // Create a dereference node
                node = malloc(sizeof(struct ASTNode));
                node->type = AST_DEREF;
                node->left = left;
                node->data_type = left->data_type - PTR;  // The data type is now the type pointed to
                break;
            default:
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
        }
    }

    return node;
}


static struct ASTNode *statement() {
    struct ASTNode *node, *condition;

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
                       "__interrupt __inportb __outportb __inportw __outportw __inportl __outport __unused void main";

    /* Read in symbols */
    int i = Break;
    while (i <= While) {
        next();
        last_identifier->tk = i++;
    }

    /* Read in keywords */
    i = INTERRUPT;
    while (i <= __UNUSED) {
        next();
        last_identifier->class = Sys;
        last_identifier->type = INT;
        last_identifier->val = i++;
    }
    i = EXIT;
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

    //print_ast(ast_root);

    generate_bytecode(ast_root);

    main_identifier->val = (int)entry;

    if(1){
        /* print out to .s file */
        FILE *f = fopen("a.s", "w");
        if (!f) {
            printf("Unable to open a.s\n");
            exit(-1);
        }
        
        write_x86(ast_root, f);

        fclose(f);

    }
    
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);

    int *pc;
    if (!(pc = (int *)main_identifier->val)) {
        printf("main() not defined\n");
        exit(-1);
    }
    
    --argc; ++argv;

    run_virtual_machine(pc, emitted_code, org_data, argc, argv);
    printf("Compilation time: %ld ns\n", diffInNanos);

    size_t code_size = last_emitted - emitted_code;
    size_t data_size = data - org_data;
    int main_offset = (int)(main_identifier->val - (int)emitted_code);
    write_bytecode(output_file, emitted_code, code_size, org_data, data_size, &main_offset);
    
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
