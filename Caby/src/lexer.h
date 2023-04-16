#pragma once

enum token_type {
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_COMMA,
    TOK_DOT,
    TOK_MINUS,
    TOK_PLUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_BANG,
    TOK_SEMICOLON,
    TOK_EQ,
    TOK_NEQ,
    TOK_LE,
    TOK_LEQ,
    TOK_GE,
    TOK_GEQ,
    TOK_ASSIGN,
    TOK_ID,
    TOK_STR,
    TOK_INT,
    TOK_AND,
    TOK_OR,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NONE,
    TOK_CLASS,
    TOK_IF,
    TOK_ELSE,
    TOK_DEF,
    TOK_RETURN,
    TOK_VAR,
    TOK_VAL,
    TOK_EOF,
    TOK_ERROR, // for error recovery.
};

struct token {
    enum token_type type;
    const char* start;
    int length;

    int row;
    int col;
};

void init_lexer(const char* source);
struct token next_token();
