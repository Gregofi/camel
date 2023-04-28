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

void dump_expr(FILE* f, struct expr* e, int spaces);

void dump_stmt(FILE* f, struct stmt* s, int spaces) {
    for (int i = 0; i < spaces; ++i) {
        fprintf(f, " ");
    }
    switch (s->k) {
    case STMT_VAR: 
    {
        struct stmt_variable* var = AS(struct stmt_variable, s);
        fprintf(f, "VARIABLE_DEF %s, mutable: %d\n", var->name.s, var->mutable);
        dump_expr(f, var->value, spaces + 1);
        break;
    }
    case STMT_ASSIGN_VAR:
    {
        struct stmt_assign_var* assign = AS(struct stmt_assign_var, s);
        fprintf(f, "VARIABLE_ASSIGN %s\n", assign->name.s);
        dump_expr(f, assign->value, spaces + 1);
        break;
    }
    case STMT_ASSIGN_LIST:
        NOT_IMPLEMENTED();
    case STMT_FUNCTION: 
    {
        struct stmt_function* fun = AS(struct stmt_function, s);
        fprintf(f, "FUNCTION_DEF %s(", fun->name.s);
        
        for (size_t i = 0; i < fun->param_len; ++i) {
            fprintf(f, "%s", fun->parameters[i].s);
            if (i != fun->param_len - 1) {
                fprintf(f, ", ");
            }
        }
        fprintf(f, "):\n");

        dump_expr(f, fun->body, spaces + 1);
        break;
    }
    case STMT_CLASS:
    {
        struct stmt_class* klass = AS(struct stmt_class, s);
        fprintf(f, "CLASS_DEF %s\n", klass->name.s);

        for (size_t i = 0; i < klass->stmts_len; ++i) {
            dump_stmt(f, (struct stmt*)(klass->statements[i]), spaces + 1);
        }
        break;
    }
    case STMT_TOP: {
        fprintf(f, "STMT_TOP:\n");
        struct stmt_top* t = AS(struct stmt_top, s);
        for (size_t i = 0; i < t->len; ++i) {
            dump_stmt(f, t->statements[i], spaces + 1);
        }
        break;
    }
    case STMT_WHILE:
    {
        fprintf(f, "STMT_WHILE:\n");
        struct stmt_while* w = AS(struct stmt_while, s);
        dump_expr(f, w->cond, spaces + 1);
        dump_expr(f, (struct expr*)w->body, spaces + 1);
        break;
    }
    case STMT_RETURN:
    {
        fprintf(f, "STMT_RETURN:\n");
        struct stmt_return* w = AS(struct stmt_return, s);
        dump_expr(f, w->value, spaces + 1);
        break;
    }
    case STMT_EXPR:
        fprintf(f, "STMT_EXPR:\n");
        dump_expr(f, AS(struct stmt_expr, s)->e, spaces + 1);
        break;
    case STMT_ASSIGN_MEMBER:
    {
        struct stmt_assign_member* w = AS(struct stmt_assign_member, s);
        fprintf(f, "STMT_ASSIGN_MEMBER %s:\n", w->member.s);
        dump_expr(f, w->target, spaces + 1);
        dump_expr(f, w->value, spaces + 1);
        break;
    }
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
