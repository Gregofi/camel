#include <ctype.h>
#include <stdalign.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "common.h"
#include "lexer.h"
#include "ast.h"
#include "memory/arena_alloc.h"

struct parser {
    struct token current;
    struct token previous;
};

struct parser parser;
/// Holds the memory in which the AST is kept.
struct ArenaAllocator* ast_heap;
struct ArenaAllocator* tmp_heap;

void error_at(const char* message) {
    fprintf(stderr, "Parser error: %s\n", message);
    exit(1);
    // TODO
}

static void advance() {
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

enum Operator tok_to_op(enum token_type tk) {
    switch (tk) {
    case TOK_PLUS: return OP_PLUS;
    case TOK_MINUS: return OP_MINUS;
    case TOK_STAR: return OP_TIMES;
    default: exit(-1); // TODO
    }
}

enum Precedence get_prec() {
    switch (parser.current.type) {
    case TOK_PLUS:
    case TOK_MINUS:
        return PREC_TERM;
    case TOK_STAR:
    case TOK_SLASH:
        return PREC_FACTOR;
    case TOK_LE:
    case TOK_LEQ:
    case TOK_GE:
    case TOK_GEQ:
        return PREC_COMPARE;
    case TOK_EQ:
    case TOK_NEQ:
        return PREC_EQ;
    default:
        return PREC_NONE;
    }
}

#define MAKE_STMT(KIND, NAME, TYPE) TYPE* NAME = (TYPE*)make_stmt(KIND, sizeof(TYPE))
struct stmt* make_stmt(enum StmtKind kind, size_t size) {
    fprintf(stderr, "LOG: %lu\n", size);
    struct stmt* s = arena_push(ast_heap, size, alignof(max_align_t));
    memset(s, 0, size);
    *s = (struct stmt){.k = kind,
                       .l = (struct loc){.row = parser.current.row,
                                         .col = parser.current.col}};
    return s;
}

#define MAKE_EXPR(KIND, NAME, TYPE) TYPE* NAME = (TYPE*)make_expr(KIND, sizeof(TYPE))
struct expr* make_expr(enum ExprKind kind, size_t size) {
    struct expr* e = arena_push(ast_heap, size, alignof(max_align_t));
    memset(e, 0, size);
    *e = (struct expr){.k = kind,
                       .l = (struct loc){.row = parser.current.row,
                                         .col = parser.current.col}};
    return e;
}

struct ostring extract_string(const struct token* tok) {
    struct ostring os;
    os.len = tok->length;

    os.s = arena_push(ast_heap, os.len + 1, alignof(os.s));
    strncpy(os.s, tok->start, os.len);
    os.s[os.len] = 0;

    return os;
}

#define STMT_BASE(ast) (&ast->s)

#define EXPR_BASE(ast) (&ast->e)

#define CURTOK() (parser.current.type)

struct expr* expr();

struct stmt* stmt();

struct expr* expr_int() {
    MAKE_EXPR(EXPR_INTEGER, e, struct expr_int);
    e->val = strtol(parser.previous.start, NULL, 10);
    return EXPR_BASE(e); 
}

struct expr* expr_grouping() {
    struct expr* e = expr();
    consume(TOK_RPAREN, "Expected closing ')'");
    return e;
}

struct expr* expr_number() {
    MAKE_EXPR(EXPR_INTEGER, e, struct expr_int);
    e->val = strtol(parser.previous.start, NULL, 10);
    return EXPR_BASE(e);
}

struct expr* expr_identifier() {
    MAKE_EXPR(EXPR_ID, e, struct expr_id);
    e->id = extract_string(&parser.previous);
    return EXPR_BASE(e);
}

struct expr* expr_string() {
    NOT_IMPLEMENTED();
}

struct expr* expr_call(struct expr* target) {
    MAKE_EXPR(EXPR_CALL, call, struct expr_call);
    call->target = target;
    
    u64 alloc_bp = arena_bp(tmp_heap);
    call->args = arena_push(tmp_heap, sizeof(*call->args), alignof(*call->args));
    while (CURTOK() != TOK_RPAREN) {
        struct expr* e = expr();
        if (e == NULL) {
            error_at(parser.current.start);
        }
        call->args[call->args_len++] = e;

        arena_push(tmp_heap, sizeof(*call->args), alignof(*call->args));
        if (CURTOK() != TOK_COMMA) {
            break;
        }
        advance();
    }

    call->args = arena_move(ast_heap, tmp_heap, alloc_bp);
    arena_restore(tmp_heap, alloc_bp);
    consume(TOK_RPAREN, "Expected ')' to close function call");
    return EXPR_BASE(call);
}

struct expr* expr_member_access(struct expr* target) {
    consume(TOK_ID, "Expected member name");
    NOT_IMPLEMENTED();
}

struct expr* expr_indexate(struct expr* target) {
    NOT_IMPLEMENTED();
}

struct expr* expr_postfix(struct expr* target) {
    switch (CURTOK()) {
    case TOK_LPAREN:
        advance();
        return expr_call(target);
    case TOK_DOT:
        advance();
        return expr_member_access(target);
    case TOK_LBRACKET:
        advance();
        return expr_indexate(target);
    default:
        return target;
    }
}

struct expr* expr_compound() {
    MAKE_EXPR(EXPR_COMPOUND, e, struct expr_compound);

    u64 alloc_bp = arena_bp(tmp_heap);
    e->stmts = arena_push(tmp_heap, sizeof(*e->stmts), alignof(*e->stmts));
    while (CURTOK() != TOK_RBRACE) {
        struct stmt* s = stmt();
        if (CURTOK() != TOK_SEMICOLON) {
            if (s->k != STMT_EXPR) {
                error_at(
                    "The compound statement must either contain an expression as "
                    "the last value or it must be terminated by a semicolon");
            }
            struct stmt_expr* se = (struct stmt_expr*)s;
            e->value = se->e;
            if (CURTOK() != TOK_RBRACE) {
                error_at("Expected closing brace");
            }
        //  free(s); s was allocated by arena allocator, so we can't free.
            break;
        }
        consume(TOK_SEMICOLON, "Expected semicolon to terminate statement"); // Eat the ;
        e->stmts[e->stmts_len++] = s;
        arena_push(tmp_heap, sizeof(*e->stmts), alignof(*e->stmts));
    }
    if (e->value == NULL) {
        MAKE_EXPR(EXPR_NONE, none, struct expr_none);
        e->value = EXPR_BASE(none);
    }
    advance(); // eat the }

    e->stmts = arena_move(ast_heap, tmp_heap, alloc_bp);
    arena_restore(tmp_heap, alloc_bp);
    return EXPR_BASE(e);
}

struct expr* expr_primary() {
    struct expr* res = NULL;
    switch (parser.current.type) {
    case TOK_LPAREN:
        advance();
        res = expr_grouping();
        break;
    case TOK_INT:
        advance();
        res = expr_number();
        break;
    case TOK_ID:
        advance();
        res = expr_identifier();
        break;
    case TOK_NONE: {
        advance();
        MAKE_EXPR(EXPR_NONE, e, struct expr_none);
        res = EXPR_BASE(e);
        break;
    }
    case TOK_LBRACE:
        advance();
        return expr_compound();
    case TOK_STR:
        res = expr_string();
        break;
    default:
        return NULL;
    }
    return expr_postfix(res);
}

struct expr* expr_unary() {
    enum token_type op_type = parser.previous.type;

    struct expr* operand = expr_primary();

    MAKE_EXPR(EXPR_UNARY, e, struct expr_unary);
    switch (op_type) {
    case TOK_MINUS: {
        e->op = OP_MINUS;
        e->operand = operand;
    }
    default:
        UNREACHABLE();        
    }
}

struct expr* expr_binary(struct expr* left, enum Precedence prec) {
    while (true) {
        enum Precedence tok_prec = get_prec();

        if (tok_prec < prec) {
            fprintf(stderr, "Returning left\n");
            return left;
        }
        fprintf(stderr, "Lower prec, continuing\n");

        MAKE_EXPR(EXPR_BINARY, merge, struct expr_binary);
        merge->left = left;

        // ParseBinOp
        merge->op = tok_to_op(parser.current.type);
        advance();

        merge->right = expr_primary();

        enum Precedence next_prec = get_prec();
        if (tok_prec < next_prec) {
            merge->right = expr_binary(merge->right, tok_prec + 1);
        }

        left = EXPR_BASE(merge);
    }
    UNREACHABLE();
}

struct expr* expr() {
    struct expr* lhs = expr_primary();
    if (lhs == NULL) {
        return NULL;
    }

    return expr_binary(lhs, PREC_BEGIN);
}

struct stmt* stmt_fun_def() {
    MAKE_STMT(STMT_FUNCTION, s, struct stmt_function);

    consume(TOK_ID, "Expected name of the function");
    s->name = extract_string(&parser.previous);

    consume(TOK_LPAREN, "Expected parameters of the function");
    size_t cap = 8;
    u64 alloc_bp = arena_bp(tmp_heap);
    s->parameters = arena_push(tmp_heap, sizeof(*s->parameters), alignof(*s->parameters));

    while (CURTOK() != TOK_RPAREN) {
        if (CURTOK() != TOK_ID) {
            error_at("Expected name of parameter");
        }

        // BUG HERE: The extract string also uses arena_push, so we cannot
        // use extend.
        s->parameters[s->param_len++] = extract_string(&parser.current);
        advance();

        if (CURTOK() == TOK_RPAREN) {
            break;
        }

        arena_push(tmp_heap, sizeof(*s->parameters), alignof(*s->parameters));

        consume(TOK_COMMA, "Expected comma to separate parameters");
    }

    consume(TOK_RPAREN, "Expected right parenthesses to close the parameters");
    consume(TOK_ASSIGN, "Expected '=' after function parameters");

    s->body = expr();

    s->parameters = arena_move(ast_heap, tmp_heap, alloc_bp);
    arena_restore(tmp_heap, alloc_bp);
    return STMT_BASE(s);
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
        advance();
        return stmt_fun_def();
    } else {
        NOT_IMPLEMENTED();
    }
}

struct stmt* top() {
    MAKE_STMT(STMT_TOP, top, struct stmt_top);

    u64 bp = arena_bp(tmp_heap);
    top->statements = arena_push(tmp_heap, sizeof(*top->statements), alignof(*top->statements));

    while (CURTOK() != TOK_EOF) {
        fprintf(stderr, "LOG size: %lu\n", top->len);
        top->statements[top->len++] = stmt();
        arena_push(tmp_heap, sizeof(*top->statements), alignof(*top->statements));
    }

    top->statements = arena_move(ast_heap, tmp_heap, bp);
    arena_restore(tmp_heap, bp); 

    return STMT_BASE(top);
}

struct stmt* parse(const char* source, struct ArenaAllocator* heap) {
    ast_heap = heap;
    struct ArenaAllocator _tmp_heap = arena_init();
    tmp_heap = &_tmp_heap;
    init_lexer(source);
    advance();
    struct stmt* result = top();
    consume(TOK_EOF, "Expected end of input.");
    arena_done(tmp_heap);
    return result;
}
