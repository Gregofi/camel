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

/// FNV-1a
static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (u8)key[i];
    hash *= 16777619;
  }
  return hash;
}

struct object_string* new_string(const char* str) {
    struct object_string* n = vmalloc(sizeof(*n));
    n->object.type = OBJECT_STRING;
    size_t len = strlen(str);
    n->size = len;
    n->hash = hashString(str, n->size);
    n->data = vmalloc(len + 1);
    memcpy(n->data, str, n->size);
    n->data[n->size] = '\0';
    return n;
}

struct object_string* new_string_move(char* str, u32 len) {
    struct object_string* n = vmalloc(sizeof(*n));
    n->object.type = OBJECT_STRING;
    n->size = len;
    n->data = str;
    n->hash = hashString(str, len);
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

struct object_function* new_function(u8 arity, u16 locals, struct bc_chunk c, u32 name) {
    struct object_function* f = vmalloc(sizeof(*f));
    f->arity = arity;
    f->locals = locals;
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
