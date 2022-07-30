#pragma once
/// Inspired by Robert Nystrom Crafting interpreters, thanks Robert!

#include "common.h"
#include "object.h"

struct entry {
    struct object_string* key;
    struct value val;
};

struct table {
    size_t count;
    size_t capacity;
    struct entry* entries;
};

void init_table(struct table* t);

void free_table(struct table* t);
