#include <stdio.h>
#include "ast.h"

#define AS(TYPE, NODE) ((TYPE*)NODE)

const char* op_to_string(enum Operator op) {
    switch (op) {
    case OP_PLUS: return "+";
    case OP_TIMES: return "*";
    case OP_MINUS: return "-";
    }
}

void dump_stmt(FILE* f, struct stmt* s, int spaces) {
    for (int i = 0; i < spaces; ++i) {
        fprintf(f, " ");
    }
    switch (s->k) {
    case STMT_VAR:
        NOT_IMPLEMENTED();
    case STMT_ASSIGN_VAR:
        NOT_IMPLEMENTED();
    case STMT_ASSIGN_LIST:
        NOT_IMPLEMENTED();
    case STMT_FUNCTION:
        NOT_IMPLEMENTED();
    case STMT_CLASS:
        NOT_IMPLEMENTED();
    case STMT_TOP: {
        fprintf(f, "STMT_TOP:\n");
        struct stmt_top* t = AS(struct stmt_top, s);
        for (size_t i = 0; i < t->len; ++i) {
            dump_stmt(f, t->statements[i], spaces + 1);
        }
        break;
    }
    case STMT_WHILE:
        NOT_IMPLEMENTED();
    case STMT_RETURN:
        NOT_IMPLEMENTED();
    case STMT_EXPR:
        fprintf(f, "STMT_EXPR:\n");
        dump_expr(f, AS(struct stmt_expr, s)->e, spaces + 1);
        break;
    case STMT_ASSIGN_MEMBER:
        NOT_IMPLEMENTED();
    }
}

void dump_expr(FILE* f, struct expr* e, int spaces) {
    for (int i = 0; i < spaces; ++i) {
        fprintf(f, " ");
    }
    switch (e->k) {
    case EXPR_INTEGER:
        fprintf(f, "EXPR_INTEGER: %d\n", AS(struct expr_int, e)->val);
        break;
    case EXPR_UNARY:
        NOT_IMPLEMENTED();
    case EXPR_BINARY: {
        struct expr_binary* bin = AS(struct expr_binary, e);
        fprintf(f, "EXPR_BINARY: %s\n", op_to_string(bin->op));
        dump_expr(f, bin->left, spaces + 1);
        dump_expr(f, bin->right, spaces + 1);
        break;
    }
    case EXPR_FLOAT:
        NOT_IMPLEMENTED();
    case EXPR_BOOL:
        fprintf(f, "EXPR_BOOL: %d\n", AS(struct expr_bool, e)->val);
        break;
    case EXPR_NONE:
        fprintf(f, "EXPR_NONE\n");
        break;
    case EXPR_STRING:
        NOT_IMPLEMENTED();
    case EXPR_LIST:
        NOT_IMPLEMENTED();
    case EXPR_ACCESS_VAR:
        NOT_IMPLEMENTED();
    case EXPR_ACCESS_LIST:
        NOT_IMPLEMENTED();
    case EXPR_ACCESS_MEMBER:
        NOT_IMPLEMENTED();
    case EXPR_IF:
        NOT_IMPLEMENTED();
    case EXPR_OP:
        NOT_IMPLEMENTED();
    case EXPR_CALL: {
        struct expr_call* call = AS(struct expr_call, e);
        fprintf(f, "CALL (args %lu):\n", call->args_len);
        dump_expr(f, call->target, spaces + 1);
        for (size_t i = 0; i < call->args_len; ++i) {
            dump_expr(f, call->args[i], spaces + 1);
        }
        break;
    }
    case EXPR_COMPOUND:
    {
        struct expr_compound* cmp = AS(struct expr_compound, e);
        fprintf(f, "COMPOUND_EXPR:\n");
        for (size_t i = 0; i < cmp->stmts_len; ++i) {
            dump_stmt(f, cmp->stmts[i], spaces + 1);
        }
        dump_expr(f, cmp->value, spaces + 1);
        break;
    }
    case EXPR_ID:
        fprintf(f, "EXPR_ID: '%s'\n", AS(struct expr_id, e)->id.s);
        break;
    }
}
