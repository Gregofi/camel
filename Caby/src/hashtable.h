#pragma once
/// Inspired by Robert Nystrom Crafting interpreters, thanks Robert!

#include "common.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

struct entry {
    struct value key;
    struct value val;
};

struct table {
    size_t count;
    size_t capacity;
    struct entry* entries;
};

void init_table(struct table* t);

void free_table(struct table* t);

/// Inserts the value under key.
/// Returns true if the key wasn't in the table, otherwise returns false.
bool table_set(struct table* t, struct value key, struct value val);

bool table_get(struct table* t, struct value key, struct value* val);

bool table_delete(struct table* t, struct value key);
