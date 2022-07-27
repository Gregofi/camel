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

/// Returns new object string with contents of 'str', the string is copied.
struct object_string* new_string(const char* str) {
    size_t len = strlen(str);
    struct object_string* n = vmalloc(sizeof(*n) + len + 1);
    n->object.type = OBJECT_STRING;
    n->size = len;
    strcpy(n->data, str);
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

struct object_function* as_function(struct object* object) {
    return (struct object_function*)object;
}

struct object_function* as_function_s(struct object* object) {
    if (object->type == OBJECT_FUNCTION) {
        return as_function(object);
    }
    return NULL;
}
