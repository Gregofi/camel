#include "test.h"
#include "src/compiler.h"
#include <string.h>

TEST(BasicCompilerTest) {
    struct stmt* s = compile("");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 0);
    
    s = compile("1");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("1 2 3 4");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 4);

    return 0;
}

TEST(BinaryExpressions) {
    struct stmt* s = compile("1 + 2");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("1 + 2 + 3");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("1 + 2 * 3");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("1 * 2 + 3");
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);
    return 0;
}

TEST(Expressions) {
    struct stmt* s = compile("(1)");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("(1) + 3");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    return 0;
}

TEST(FunctionCall) {
    struct stmt* s = compile("foo()");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("foo(1)");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("foo(1, 2)");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = compile("foo(1 + 2 * 3, 4 + 5)");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("foo(0,1,2,3,4,5,6,7,8,9,10,11)");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("3 + foo()");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);
    return 0;
}

TEST(CompoundStatements) {
    struct stmt* s = compile("{}");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("{1}");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("{1; 2}");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = compile("{1;}");
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);
    return 0;
}

int main(void) {
    RUN_TEST(BasicCompilerTest);
    RUN_TEST(BinaryExpressions);
    RUN_TEST(Expressions);
    RUN_TEST(FunctionCall);
    RUN_TEST(CompoundStatements);
}
