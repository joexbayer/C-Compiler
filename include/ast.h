#ifndef __AST_H
#define __AST_H

#include <cc.h>

/* AST Node types */
enum ast_nodeType {
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
    AST_ADJ,
    AST_ASM
};

/* AST Node */
struct ast_node {
    enum ast_nodeType type;
    int value;
    int data_type;
    struct ast_node *left;
    struct ast_node *right;
    struct ast_node *next;
    struct identifier ident;
    struct member *member;
    char* asm_code;
};

#endif // !__AST_H
