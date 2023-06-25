#include "test.h"
#include "src/memory/arena_alloc.h"
#include "src/parser.h"
#include <string.h>

TEST(BasicCompilerTest) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 0);
    
    s = parse("1", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("1 2 3 4", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 4);

    arena_done(&alloc);
    return 0;
}

TEST(BinaryExpressions) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("1 + 2", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("1 + 2 + 3", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("1 + 2 * 3", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("1 * 2 + 3", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ(s->k, STMT_TOP);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    arena_done(&alloc);
    return 0;
}

TEST(Expressions) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("(1)", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("(1) + 3", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    arena_done(&alloc);
    return 0;
}

TEST(FunctionCall) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("foo()", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("foo(1)", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("foo(1, 2)", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("foo(1 + 2 * 3, 4 + 5)", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("foo(0,1,2,3,4,5,6,7,8,9,10,11)", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("3 + foo()", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    arena_done(&alloc);
    return 0;
}

TEST(CompoundStatements) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("{}", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("{1}", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("{1; 2; 3}", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);
    dump_stmt(stderr, s, 0);

    s = parse("{1;}", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    arena_done(&alloc);
    return 0;
}

TEST(FunctionDef) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("def foo() = 1", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("def foo(a, b, c) = a + b", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse(
"def foo(a) = a\n"
"1 + foo()\n", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 2);

    arena_done(&alloc);
    return 0;
}

TEST(VarDecls) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("var x = 5 val y = 6 x = 1", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 3);

    arena_done(&alloc);
    return 0;
}

TEST(ClassDecl) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* s = parse("class foo { }", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    s = parse("class foo { def bar() = 1 }", &alloc);
    ASSERT_W(s != NULL);
    ASSERT_EQ((int)((struct stmt_top*)s)->len, 1);

    arena_done(&alloc);
    return 0;
}

int main(void) {
    RUN_TEST(BasicCompilerTest);
    RUN_TEST(BinaryExpressions);
    RUN_TEST(Expressions);
    RUN_TEST(FunctionCall);
    RUN_TEST(CompoundStatements);
    RUN_TEST(FunctionDef);
    RUN_TEST(VarDecls);
    RUN_TEST(ClassDecl);
}
