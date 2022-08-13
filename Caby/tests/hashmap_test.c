#include <stdbool.h>

#include "test.h"
#include "../src/hashtable.h"

TEST(HashMapBasic) {
    struct table t;
    init_table(&t);
    ASSERT_W(t.capacity == 0);
    ASSERT_W(t.count == 0);

    struct value val_out;

    struct value key1 = NEW_OBJECT(new_string("Hello, World!"));
    table_set(&t, key1, NEW_INT(3));
    ASSERT_W(t.count == 1);
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT);
    ASSERT_W(val_out.integer == 3);

    ASSERT_W(!table_set(&t, key1, NEW_INT(5)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT);
    ASSERT_W(val_out.integer == 5);

    struct value key2 = NEW_OBJECT(new_string("FOO"));
    struct value key3 = NEW_OBJECT(new_string("BAR"));
    struct value key4 = NEW_OBJECT(new_string("BAZ"));
    struct value key5 = NEW_INT(1);
    struct value key6 = NEW_INT(2);

    ASSERT_W(table_set(&t, key2, NEW_BOOL(true)));
    ASSERT_W(table_set(&t, key3, NEW_NONE()));
    ASSERT_W(table_set(&t, key4, NEW_DOUBLE(4.2)));
    ASSERT_W(table_set(&t, key5, NEW_INT(42)));
    ASSERT_W(table_set(&t, key6, key1));

    // Check that the keys exists
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);

    // Check reads/writes through equal bot not same "object" keys
    struct value key7_1 = NEW_OBJECT(new_string("FOOBAR"));
    struct value key7_2 = NEW_OBJECT(new_string("FOOBAR"));
    ASSERT_W(table_set(&t, key7_1, NEW_INT(9)));
    ASSERT_W(table_get(&t, key7_2, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 9);
    ASSERT_W(table_get(&t, key7_1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 9);
    ASSERT_W(!table_set(&t, key7_2, NEW_DOUBLE(9.9)));
    ASSERT_W(table_get(&t, key7_1, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 9.9);
    ASSERT_W(table_get(&t, key7_2, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 9.9);

    // Delete the above entry
    ASSERT_W(table_delete(&t, key7_1));
    ASSERT_W(!table_delete(&t, key7_1));
    ASSERT_W(!table_delete(&t, key7_2));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);


    // Add a couple more entries to stress reallocation
    struct value key7 = NEW_DOUBLE(1.111);
    struct value key8 = NEW_OBJECT(new_string("key8"));
    struct value key9 = NEW_BOOL(true);
    struct value key10 = NEW_BOOL(false);

    ASSERT_W(table_set(&t, key7, NEW_INT(70)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(!table_get(&t, key8, &val_out));
    ASSERT_W(!table_get(&t, key9, &val_out));
    ASSERT_W(!table_get(&t, key10, &val_out));


    ASSERT_W(table_set(&t, key8, NEW_INT(80)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(!table_get(&t, key9, &val_out));
    ASSERT_W(!table_get(&t, key10, &val_out));

    ASSERT_W(table_set(&t, key9, NEW_INT(90)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(!table_get(&t, key10, &val_out));

    ASSERT_W(table_set(&t, key10, NEW_INT(100)));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(table_get(&t, key2, &val_out));
    ASSERT_W(val_out.type == VAL_BOOL && val_out.boolean == true);
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    // Delete all entries one by one
    ASSERT_W(table_delete(&t, key2));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(table_get(&t, key3, &val_out));
    ASSERT_W(val_out.type == VAL_NONE);
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(table_get(&t, key1, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 5);
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(table_get(&t, key4, &val_out));
    ASSERT_W(val_out.type == VAL_DOUBLE && val_out.double_num == 4.2);
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(table_get(&t, key5, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 42);
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(table_get(&t, key6, &val_out));
    ASSERT_W(val_out.type == VAL_OBJECT && val_out.object == key1.object);
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(!table_get(&t, key6, &val_out));
    ASSERT_W(table_get(&t, key7, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 70);
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key7));
    ASSERT_W(!table_delete(&t, key7));
    ASSERT_W(!table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(!table_get(&t, key6, &val_out));
    ASSERT_W(!table_get(&t, key7, &val_out));
    ASSERT_W(table_get(&t, key8, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 80);
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key8));
    ASSERT_W(!table_delete(&t, key8));
    ASSERT_W(!table_delete(&t, key7));
    ASSERT_W(!table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(!table_get(&t, key6, &val_out));
    ASSERT_W(!table_get(&t, key7, &val_out));
    ASSERT_W(!table_get(&t, key8, &val_out));
    ASSERT_W(table_get(&t, key9, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 90);
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);

    ASSERT_W(table_delete(&t, key9));
    ASSERT_W(!table_delete(&t, key9));
    ASSERT_W(!table_delete(&t, key8));
    ASSERT_W(!table_delete(&t, key7));
    ASSERT_W(!table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(!table_get(&t, key6, &val_out));
    ASSERT_W(!table_get(&t, key7, &val_out));
    ASSERT_W(!table_get(&t, key8, &val_out));
    ASSERT_W(!table_get(&t, key9, &val_out));
    ASSERT_W(table_get(&t, key10, &val_out));
    ASSERT_W(val_out.type == VAL_INT && val_out.integer == 100);


    ASSERT_W(table_delete(&t, key10));
    ASSERT_W(!table_delete(&t, key9));
    ASSERT_W(!table_delete(&t, key8));
    ASSERT_W(!table_delete(&t, key7));
    ASSERT_W(!table_delete(&t, key6));
    ASSERT_W(!table_delete(&t, key5));
    ASSERT_W(!table_delete(&t, key4));
    ASSERT_W(!table_delete(&t, key1));
    ASSERT_W(!table_delete(&t, key3));
    ASSERT_W(!table_delete(&t, key2));
    ASSERT_W(!table_get(&t, key1, &val_out));
    ASSERT_W(!table_get(&t, key2, &val_out));
    ASSERT_W(!table_get(&t, key3, &val_out));
    ASSERT_W(!table_get(&t, key4, &val_out));
    ASSERT_W(!table_get(&t, key5, &val_out));
    ASSERT_W(!table_get(&t, key6, &val_out));
    ASSERT_W(!table_get(&t, key7, &val_out));
    ASSERT_W(!table_get(&t, key8, &val_out));
    ASSERT_W(!table_get(&t, key9, &val_out));
    ASSERT_W(!table_get(&t, key10, &val_out));

    return 0;
}

int main() {
    RUN_TEST(HashMapBasic);
}
