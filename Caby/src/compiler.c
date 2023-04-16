#include "compiler.h"
#include "common.h"
#include "lexer.h"
#include "ast.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct parser {
    struct token current;
    struct token previous;
};

struct parser parser;

void error_at(const char* begin) {
    fprintf(stderr, "Parser error\n");
    exit(1);
    // TODO
}

void advance() {
    parser.previous = parser.current;

    for(;;) {
        parser.current = next_token();
        if (parser.current.type != TOK_ERROR) {
            break;
        }

        error_at(parser.current.start);
    }
}

void consume(enum token_type kind, const char* message) {
    if (parser.current.type == kind) {
        advance();
    } else {
        error_at(message);
    }
}

#define MAKE_STMT(KIND, NAME, TYPE) TYPE* NAME = (TYPE*)make_stmt(KIND, sizeof(TYPE))
struct stmt* make_stmt(enum StmtKind kind, size_t size) {
    struct stmt* s = malloc(sizeof(size));
    *s = (struct stmt){.k = kind,
                       .l = (struct loc){.row = parser.current.row,
                                         .col = parser.current.col}};
    return s;
}

#define STMT_BASE(ast) (&ast->s)

#define EXPR_BASE(ast) (&ast->e)

#define CURTOK() (parser.current.type)

/// Tries to parse an expression, returns NULL if
/// the lookahead is not an expression complying.
struct expr* expr() {
    
}

struct stmt* stmt() {
    // Try to parse the stmt as an expression,
    // return it if it succeeds.
    struct expr* e = expr();
    if (e != NULL) {
        MAKE_STMT(STMT_EXPR, expr_stmt, struct stmt_expr);
        expr_stmt->e = e;
        return STMT_BASE(expr_stmt);
    }

    if (CURTOK() == TOK_DEF) {
        NOT_IMPLEMENTED();
    } else {
        NOT_IMPLEMENTED();
    }
}

struct stmt* top() {
    MAKE_STMT(STMT_TOP, top, struct stmt_top);

    size_t cap = 8;
    top->statements = malloc(cap * sizeof(*top->statements));

    while (CURTOK() != TOK_EOF) {
        top->statements[top->len++] = *stmt();
        handle_capacity(top->statements, top->len, &cap, sizeof(*top->statements));
    }

    return STMT_BASE(top);
}

void compile(const char* source) {
    init_lexer(source);
    advance();
    top();
    consume(TOK_EOF, "Expected end of input.");
}
