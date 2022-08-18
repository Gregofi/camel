#include "hashtable.h"
#include "common.h"
#include "object.h"
#include <stdbool.h>

void init_table(struct table* t) {
    memset(t, 0, sizeof(*t));
}

void free_table(struct table* t) {
    for (size_t i = 0; i < t->capacity; ++i) {
        free_value(&t->entries[i].key);
        free_value(&t->entries[i].val);
    }
    free(t->entries);
    init_table(t);
}

/// Returns the entry where the key should be inserted if it's not in table,
/// otherwise returns entry saved under the key.
static struct entry* find_entry(struct entry* entries, size_t capacity,
                                struct value key) {
    // Assume the capacity is a power of two and do fast modulo using and.
    u32 hash = value_hash(key);
    u32 idx = hash & (capacity - 1);
    struct entry* tombstone = NULL;

    for (;;) {
        struct entry* e = &entries[idx];
        if (e->key.type == VAL_NONE) {
            // empty entry
            if (e->val.type == VAL_NONE) {
                return tombstone != NULL ? tombstone : e;
            } else {
                // tombstone
                // Save first occurence of encountered tombstone
                if (tombstone == NULL) {
                    tombstone = e;
                }
            }
        } else if (value_eq(e->key, key)) {
            return e;
        }

        idx = (idx + 1) & (capacity - 1);
    }
    UNREACHABLE();
}

static void adjust_capacity(struct table* t, size_t capacity) {
    struct entry* entries = vmalloc(sizeof(*entries) * capacity);

    for (size_t i = 0; i < capacity; ++i) {
        entries[i].key = NEW_NONE();
        entries[i].val = NEW_NONE();
    }
    t->count = 0;

    // Rehash the entire table
    for (size_t i = 0; i < t->capacity; ++i) {
        struct entry* e = &t->entries[i];
        if (e->key.type == VAL_NONE) {
            continue;
        }

        struct entry* dest = find_entry(entries, capacity, e->key);
        dest->key = e->key;
        dest->val = e->val;
        t->count += 1;
    }
    vfree(t->entries);

    t->entries = entries;
    t->capacity = capacity;
}

bool table_set(struct table* t, struct value key, struct value val) {
    if (t->count >= t->capacity * TABLE_MAX_LOAD) {
        u64 capacity = get_cap(t->capacity);
        adjust_capacity(t, capacity);
    }

    struct entry* e = find_entry(t->entries, t->capacity, key);
    bool is_new_key = e->key.type == VAL_NONE;
    // Only increment count if the bucket doesn't contain tombstone
    if (is_new_key && e->val.type == VAL_NONE) {
        t->count += 1;
    }

    e->key = key;
    e->val = val;
    return is_new_key;
}

bool table_get(struct table* t, struct value key, struct value* val) {
    if (t->count == 0) {
        return false;
    }

    struct entry* e = find_entry(t->entries, t->capacity, key);
    if (e->key.type == VAL_NONE) {
        return false;
    }

    *val = e->val;
    return true;
}

bool table_delete(struct table* t, struct value key) {
    if (t->count == 0) {
        return false;
    }

    struct entry* e = find_entry(t->entries, t->capacity, key);
    if (e->key.type == VAL_NONE) {
        return false;
    }

    // tombstone
    e->key.type = VAL_NONE;
    e->val = NEW_BOOL(true);
    return true;
}
