#include "test.h"
#include "src/memory/arena_alloc.h"
#include <stdalign.h>

TEST(BasicAlloc) {
    struct ArenaAllocator heap = arena_init();
    char* c1 = arena_push(&heap, 1, alignof(char));
    *c1 = 'A';
    char* c2 = arena_push(&heap, 1, alignof(char));
    *c2 = 'B';
    char* c3 = arena_push(&heap, 3, alignof(char));
    c3[0] = 'C';
    c3[1] = 'D';
    c3[2] = 'E';
    ASSERT_EQ(*c1, 'A');
    ASSERT_EQ(*c2, 'B');
    ASSERT_EQ(c3[0], 'C');

    arena_push(&heap, 2, alignof(char));
    c3[3] = 'F';
    c3[4] = 'G';

    char* c4 = arena_push(&heap, 1, alignof(char));
    *c4 = 'H';

    ASSERT_EQ(*c1, 'A');
    ASSERT_EQ(*c2, 'B');
    ASSERT_EQ(c3[0], 'C');
    ASSERT_EQ(c3[1], 'D');
    ASSERT_EQ(c3[2], 'E');
    ASSERT_EQ(c3[3], 'F');
    ASSERT_EQ(c3[4], 'G');
    ASSERT_EQ(*c4, 'H');

    arena_done(&heap);
    return 0;
}

int main(void) {
    RUN_TEST(BasicAlloc);
}
