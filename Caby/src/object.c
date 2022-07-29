#include "object.h"

bool is_object_type(struct value* val, enum object_type type) {
    return val->type == VAL_OBJECT && val->object->type == type;
}

struct object* to_object(struct value val) {
    return val.object;
}

struct object* to_object_s(struct value val) {
    if (val.type == VAL_OBJECT) {
        return val.object;
    }
    return NULL;
}

struct object_string* new_string(const char* str) {
    size_t len = strlen(str);
    struct object_string* n = vmalloc(sizeof(*n) + len + 1);
    n->object.type = OBJECT_STRING;
    n->size = len;
    strcpy(n->data, str);
    return n;
}

struct object_string* new_string_empty(size_t str_len) {
    struct object_string* n = vmalloc(sizeof(*n) + str_len + 1);
    n->object.type = OBJECT_STRING;
    n->size = str_len;
    n->data[0] = '\0';
    return n;
}

struct object_string* as_string(struct object* object) {
    return (struct object_string*)object;
}

struct object_string* as_string_s(struct object* object) {
    if (object->type == OBJECT_STRING) {
        return as_string(object);
    }
    return NULL;
}

struct object_function* new_function(u8 arity, struct bc_chunk c, u32 name) {
    struct object_function* f = vmalloc(sizeof(*f));
    f->arity = arity;
    f->object.type = OBJECT_FUNCTION;
    f->bc = c;
    f->name = name;
    return f;
}

struct object_function* as_function(struct object* object) {
    return (struct object_function*)object;
}

struct object_function* as_function_s(struct object* object) {
    if (object->type == OBJECT_FUNCTION) {
        return as_function(object);
    }
    return NULL;
}
