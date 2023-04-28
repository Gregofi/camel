#include "lexer.h"
#include "src/common.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct lexer {
    const char* begin;
    const char* curr;
    int row;
    int col;
};

struct lexer lexer;

void init_lexer(const char* source) {
    lexer.begin = source;
    lexer.curr = source;
    lexer.row = 1;
    lexer.col = 0;
}

static char advance() {
    lexer.col += 1;
    return *lexer.curr++;
}

char peek() {
    return *lexer.curr;
}

char next() {
    lexer.col += 1;
    return *++lexer.curr;
}

bool input_end() {
    return peek() == '\0';
}

struct token make_token(enum token_type type) {
    struct token tok;
    tok.type = type;
    tok.start = lexer.begin;
    tok.length = (int)(lexer.curr - lexer.begin);
    tok.row = lexer.row;
    return tok;
}

/// The message should have static lifetime.
struct token make_error(const char* message) {
    struct token tok;
    tok.type = TOK_ERROR;
    tok.start = message;
    tok.length = strlen(message);
    tok.row = lexer.row;
    return tok;
}

bool match(char exp) {
    if (input_end()) {
        return false;
    } else if (*lexer.curr != exp) {
        return false;
    } else {
        advance();
        return true;
    }
}

void skip_whitespace() {
    char c = peek();
    // TODO: Skip semicolons for now
    while (isspace(c)) {
        if (c == '\n') {
            lexer.row += 1;
            lexer.col = 0;
        }
        c = next();
    }
}

struct token make_string() {
    while (peek() != '"' && !input_end()) {
        advance();
    }

    if (input_end()) {
        return make_error("Unterminated string literal");
    }

    advance(); // eat '"'
    return make_token(TOK_STR);
}

struct token make_number() {
    while (isdigit(peek())) {
        advance();
    }
    // TODO: Handle floats
    return make_token(TOK_INT);
}

struct token make_id() {
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }

    int length = lexer.curr - lexer.begin;
    if (!strncmp(lexer.begin, "if", length)) {
        return make_token(TOK_IF);
    } else if (str_eq("class", lexer.begin, length)) {
        return make_token(TOK_CLASS);
    } else if (!strncmp(lexer.begin, "def", length)) {
        return make_token(TOK_DEF);
    } else if (!strncmp(lexer.begin, "return", length)) {
        return make_token(TOK_RETURN);
    } else if (!strncmp(lexer.begin, "else", length)) {
        return make_token(TOK_ELSE);
    } else if (!strncmp(lexer.begin, "true", length)) {
        return make_token(TOK_TRUE);
    } else if (!strncmp(lexer.begin, "false", length)) {
        return make_token(TOK_FALSE);
    } else if (!strncmp(lexer.begin, "while", length)) {
        return make_token(TOK_WHILE);
    } else if (!strncmp(lexer.begin, "val", length)) {
        return make_token(TOK_VAL);
    } else if (!strncmp(lexer.begin, "var", length)) {
        return make_token(TOK_VAR);
    } else if (!strncmp(lexer.begin, "none", length)) {
        return make_token(TOK_NONE);
    } else {
        return make_token(TOK_ID);
    }
}

struct token next_token() {
    skip_whitespace();
    lexer.begin = lexer.curr;
    if (input_end()) {
        return make_token(TOK_EOF);
    }

    char c = advance();
    if (c == '(') {
        return make_token(TOK_LPAREN);
    } else if (c == ')') {
        return make_token(TOK_RPAREN);
    } else if (c == '{') {
        return make_token(TOK_LBRACE);
    } else if (c == '}') {
        return make_token(TOK_RBRACE);
    } else if (c == '+') {
        return make_token(TOK_PLUS);
    } else if (c == '-') {
        return make_token(TOK_MINUS);
    } else if (c == ';') {
        return make_token(TOK_SEMICOLON);
    } else if (c == ',') {
        return make_token(TOK_COMMA);
    } else if (c == '.') {
        return make_token(TOK_DOT);
    } else if (c == '/') {
        return make_token(TOK_SLASH);
    } else if (c == '*') {
        return make_token(TOK_STAR);
    } else if (c == '=') {
        if (match('=')) {
            return make_token(TOK_EQ);
        }
        return make_token(TOK_ASSIGN);
    } else if (c == '!') {
        if (match('=')) {
            return make_token(TOK_NEQ);
        }
        return make_token(TOK_BANG);
    } else if (c == '<') {
        if (match('=')) {
            return make_token(TOK_LEQ);
        }
        return make_token(TOK_LE);
    } else if (c == '>') {
        if (match('=')) {
            return make_token(TOK_GEQ);
        }
        return make_token(TOK_GE);
    } else if (c == '"') {
        return make_string();
    } else if (isdigit(c)) {
        return make_number();
    } else if (isalpha(c) || c == '_') {
        return make_id();
    } else {
        return make_error("Unknown token");
    }
}

const char* tok_to_string(enum token_type tk) {
    switch (tk) {
    case TOK_LPAREN: return "TOK_LPAREN";
    case TOK_RPAREN: return "TOK_RPAREN";
    case TOK_LBRACE: return "TOK_LBRACE";
    case TOK_RBRACE: return "TOK_RBRACE";
    case TOK_LBRACKET: return "TOK_LBRACKET";
    case RBRACKET: return "RBRACKET";
    case TOK_COMMA: return "TOK_COMMA";
    case TOK_DOT: return "TOK_DOT";
    case TOK_MINUS: return "TOK_MINUS";
    case TOK_PLUS: return "TOK_PLUS";
    case TOK_STAR: return "TOK_STAR";
    case TOK_SLASH: return "TOK_SLASH";
    case TOK_BANG: return "TOK_BANG";
    case TOK_SEMICOLON: return "TOK_SEMICOLON";
    case TOK_EQ: return "TOK_EQ";
    case TOK_NEQ: return "TOK_NEQ";
    case TOK_LE: return "TOK_LE";
    case TOK_LEQ: return "TOK_LEQ";
    case TOK_GE: return "TOK_GE";
    case TOK_GEQ: return "TOK_GEQ";
    case TOK_ASSIGN: return "TOK_ASSIGN";
    case TOK_ID: return "TOK_ID";
    case TOK_STR: return "TOK_STR";
    case TOK_INT: return "TOK_INT";
    case TOK_AND: return "TOK_AND";
    case TOK_OR: return "TOK_OR";
    case TOK_TRUE: return "TOK_TRUE";
    case TOK_FALSE: return "TOK_FALSE";
    case TOK_NONE: return "TOK_NONE";
    case TOK_CLASS: return "TOK_CLASS";
    case TOK_IF: return "TOK_IF";
    case TOK_ELSE: return "TOK_ELSE";
    case TOK_DEF: return "TOK_DEF";
    case TOK_RETURN: return "TOK_RETURN";
    case TOK_VAR: return "TOK_VAR";
    case TOK_VAL: return "TOK_VAL";
    case TOK_EOF: return "TOK_EOF";
    case TOK_WHILE: return "TOK_WHILE";
    case TOK_ERROR: return "TOK_ERROR";
    }
}
