#include "lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct lexer {
    const char* begin;
    const char* curr;
    int line;
};

struct lexer lexer;

void init_lexer(const char* source) {
    lexer.begin = source;
    lexer.curr = source;
    lexer.line = 1;
}

char advance() {
    return *lexer.curr++;
}

char peek() {
    return *lexer.curr;
}

char next() {
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
    tok.line = lexer.line;
    return tok;
}

/// The message should have static lifetime.
struct token make_error(const char* message) {
    struct token tok;
    tok.type = TOK_ERROR;
    tok.start = message;
    tok.length = strlen(message);
    tok.line = lexer.line;
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
    while (isspace(c)) {
        if (c == '\n') {
            lexer.line += 1;
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
