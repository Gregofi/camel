#include "test.h"
#include "../src/hashtable.h"

TEST(HashMapBasic) {
    struct table t;
    init_table(&t);
    ASSERT_W(t.capacity == 0);
    ASSERT_W(t.count == 0);

    struct value val_out;

    struct object_string* key1 = new_string("Hello, World!");
    table_set(&t, key1, NEW_INT(3));
    ASSERT_W(t.count == 1);
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT);
    ASSERT_W(val_out.integer == 3);

    ASSERT_W(!table_set(&t, key1, NEW_INT(5)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT);
    ASSERT_W(val_out.integer == 5);

    struct object_string* key2 = new_string("FOO");
    struct object_string* key3 = new_string("BAR");
    struct object_string* key4 = new_string("BAZ");
    ASSERT_W(table_set(&t, key2, NEW_BOOL(true)));
    ASSERT_W(table_set(&t, key3, NEW_NONE()));
    ASSERT_W(table_set(&t, key4, NEW_DOUBLE(4.2)));

    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    
    ASSERT_W(table_delete(&t, key2));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);

    ASSERT_W(table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);

    ASSERT_W(table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(table_get(&t, key4, &val_out));

    ASSERT_W(table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));

    return 0;
}

int main() {
    RUN_TEST(HashMapBasic);
}
