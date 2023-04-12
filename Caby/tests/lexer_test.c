#include "test.h"
#include "src/lexer.h"
#include <string.h>

TEST(LexerBasic) {
    init_lexer("( + }");
    struct token t = next_token();
    ASSERT_EQ(t.type, TOK_LPAREN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_PLUS);
    t = next_token();
    ASSERT_EQ(t.type, TOK_RBRACE);
    t = next_token();
    ASSERT_EQ(t.type, TOK_EOF);

    init_lexer("( = >=");
    t = next_token();
    ASSERT_EQ(t.type, TOK_LPAREN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_ASSIGN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_GEQ);
    t = next_token();
    ASSERT_EQ(t.type, TOK_EOF);

    init_lexer("if ( 1 >= 2 ) { none };");
    t = next_token();
    ASSERT_EQ(t.type, TOK_IF);
    t = next_token();
    ASSERT_EQ(t.type, TOK_LPAREN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_INT);
    t = next_token();
    ASSERT_EQ(t.type, TOK_GEQ);
    t = next_token();
    ASSERT_EQ(t.type, TOK_INT);
    t = next_token();
    ASSERT_EQ(t.type, TOK_RPAREN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_LBRACE);
    t = next_token();
    ASSERT_EQ(t.type, TOK_NONE);
    t = next_token();
    ASSERT_EQ(t.type, TOK_RBRACE);
    t = next_token();
    ASSERT_EQ(t.type, TOK_SEMICOLON);
    t = next_token();
    ASSERT_EQ(t.type, TOK_EOF);
    return 0;

    init_lexer("val x = \"Hello\"");
    t = next_token();
    ASSERT_EQ(t.type, TOK_VAL);
    t = next_token();
    ASSERT_EQ(t.type, TOK_ID);
    ASSERT_EQ(t.length, 1);
    ASSERT_W(!strcmp(t.start, "x"));
    t = next_token();
    ASSERT_EQ(t.type, TOK_ASSIGN);
    t = next_token();
    ASSERT_EQ(t.type, TOK_STR);
    ASSERT_EQ(t.length, 7);
    ASSERT_W(!strcmp(t.start, "\"Hello\""));
    t = next_token();
    ASSERT_EQ(t.type, TOK_EOF);
}

int main() {
    RUN_TEST(LexerBasic);
}
