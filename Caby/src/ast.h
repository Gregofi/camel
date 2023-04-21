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
    EXPR_ACCESS_VAR,
    EXPR_ACCESS_LIST,
    EXPR_ACCESS_MEMBER,
    EXPR_IF,
    EXPR_OP,
    EXPR_CALL,
    EXPR_COMPOUND,
};

enum Operator {
    OP_PLUS,
    OP_TIMES,
    OP_MINUS,
};

enum Precedence {
    PREC_NONE,
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
    struct stmt* statements;
    size_t len;
};

struct stmt_expr {
    struct stmt s;
    struct expr* e;
};

struct stmt_function {
    const char* name;
    const char** parameters;
    struct expr* body;
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

struct expr_compound {
    struct expr e;
    struct stmt* stmts;
    struct expr* last_expr;
    size_t len;
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
