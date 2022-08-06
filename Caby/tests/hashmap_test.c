#include "test.h"
#include "../src/hashtable.h"

TEST(HashMapBasic) {
    struct table t;
    init_table(&t);
    ASSERT_W(t.capacity == 0);
    ASSERT_W(t.count == 0);

    struct value val_out;
    bool exists;

    struct object_string* key1 = new_string("Hello, World!");
    table_set(&t, key1, NEW_INT(3));
    ASSERT_W(t.count == 1);
    exists = table_get(&t, key1, &val_out);
    ASSERT_W(exists);
    ASSERT_W(val_out.type == VAL_INT);
    ASSERT_W(val_out.integer == 3);
    return 0;
}



int main() {
    RUN_TEST(HashMapBasic);
}
