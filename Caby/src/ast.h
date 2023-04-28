#pragma once
#include "src/common.h"
#include "src/lexer.h"
#include <stdbool.h>
#include <stddef.h>

enum StmtKind {
    STMT_VAR,
    STMT_ASSIGN_VAR,
    STMT_ASSIGN_LIST,
    STMT_FUNCTION,
    STMT_CLASS,
    STMT_TOP,
    STMT_WHILE,
    STMT_RETURN,
    STMT_EXPR,
    STMT_ASSIGN_MEMBER,
};

enum ExprKind {
    EXPR_INTEGER,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_FLOAT,
    EXPR_BOOL,
    EXPR_NONE,
    EXPR_STRING,
    EXPR_LIST,
    EXPR_ACCESS_LIST,
    EXPR_ACCESS_MEMBER,
    EXPR_IF,
    EXPR_OP,
    EXPR_CALL,
    EXPR_COMPOUND,
    EXPR_ID,
};

enum Operator {
    OP_PLUS,
    OP_TIMES,
    OP_MINUS,
};

enum Precedence {
    PREC_NONE, // Indicates that the current token is not binary op.
    PREC_BEGIN, // The initial precedence.
    PREC_ASSIGN,
    PREC_OR,
    PREC_AND,
    PREC_EQ, // != ==
    PREC_COMPARE, // < > <= >=
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // ! -
    PREC_CALL, // . ()
    PREC_PRIMARY,
};

typedef struct expr* (*ParseFn)();

struct parse_rule {
    // ParseFn prefix;
    // ParseFn infix;
    // Precedence prec;
};

struct loc {
    int row;
    int col;
};

struct stmt {
    struct loc l;
    enum StmtKind k;
};

struct expr {
    struct loc l;
    enum ExprKind k;
};

struct stmt_top {
    struct stmt s;
    struct stmt** statements;
    size_t len;
};

struct stmt_expr {
    struct stmt s;
    struct expr* e;
};

struct stmt_function {
    struct stmt s;
    struct ostring name;
    struct ostring* parameters;
    size_t param_len;
    struct expr* body;
};

struct stmt_variable {
    struct stmt s;
    struct ostring name;
    bool mutable;
    struct expr* value;
};

struct stmt_assign_var {
    struct stmt s;
    struct ostring name;
    struct expr* value;
};

struct stmt_assign_list {
    struct stmt s;
    struct expr* list;
    struct expr* index;
    struct expr* value;
};

struct stmt_class {
    struct stmt s;
    struct ostring name;
    struct stmt_function** statements;
    size_t stmts_len;
};

struct stmt_while {
    struct stmt s;
    struct expr* cond;
    struct expr_compound* body;
};

struct stmt_return {
    struct stmt s;
    struct expr* value;
};

struct stmt_assign_member {
    struct stmt s;
    struct expr* target;
    struct ostring member;
    struct expr* value;
};

struct expr_none {
   struct expr e; 
};

struct expr_int {
    struct expr e;
    int val;
};

struct expr_float {
    struct expr e;
    double val;
};

struct expr_bool {
    struct expr e;
    bool val;
};

struct expr_call {
    struct expr e;
    struct expr* target;
    struct expr** args;
    size_t args_len;
};

struct expr_compound {
    struct expr e;
    struct stmt** stmts;
    size_t stmts_len;
    struct expr* value;
};

struct expr_if {
    struct expr e;
    struct expr* cond;
    struct expr* true_b; 
    /// Can be NULL! Meaning that the false branch does not exist.
    struct expr* false_b;
};

struct expr_unary {
    struct expr e;
    struct expr* operand;
    enum Operator op;
};

struct expr_binary {
    struct expr e;
    struct expr* left;
    enum Operator op;
    struct expr* right;
};

struct expr_id {
    struct expr e;
    struct ostring id;
};

void dump_stmt(FILE* f, struct stmt* s, int spaces);
void dump_expr(FILE* f, struct expr* s, int spaces);

const char* op_to_string(enum Operator op);
