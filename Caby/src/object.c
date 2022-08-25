#include "object.h"
#include "bytecode.h"
#include "vm.h"

#include <assert.h>

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
static u32 hashString(const char* key, int length) {
  u32 hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (u8)key[i];
    hash *= 16777619;
  }
  return hash;
}

static void init_object(vm_t* vm, struct object* obj, enum object_type type) {
    obj->type = type;
    obj->next = vm->objects;
    vm->objects = obj;
    obj->gc_data = 0;
}

struct object_string* new_string(vm_t* vm, const char* str) {
    struct object_string* n = vmalloc(vm, sizeof(*n));
    size_t len = strlen(str);
    n->size = len;
    n->hash = hashString(str, n->size);
    n->data = vmalloc(vm, len + 1);
    memcpy(n->data, str, n->size);
    n->data[n->size] = '\0';
    init_object(vm, &n->object, OBJECT_STRING);
    return n;
}

struct object_string* new_string_move(vm_t* vm, char* str, u32 len) {
    struct object_string* n = vmalloc(vm, sizeof(*n));
    n->size = len;
    n->data = str;
    n->hash = hashString(str, len);
    init_object(vm, &n->object, OBJECT_STRING);
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

struct object_function* new_function(vm_t* vm, u8 arity, u16 locals, struct bc_chunk c, u32 name) {
    struct object_function* f = vmalloc(vm, sizeof(*f));
    init_object(vm, &f->object, OBJECT_FUNCTION);
    f->arity = arity;
    f->locals = locals;
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

struct object_native* new_native(vm_t* vm, native_fn_t fun) {
    struct object_native* native = vmalloc(vm, sizeof(*native));
    init_object(vm, &native->object, OBJECT_NATIVE);
    native->function = fun;
    return native;
}

struct object_native* as_native(struct object* object) {
    return (struct object_native*)object;
}

struct object_native* as_native_s(struct object* object) {
    if (object->type == OBJECT_NATIVE) {
        return as_native(object);
    }
    return NULL;
}

void free_object(struct object* obj) {
    switch (obj->type) {
        case OBJECT_STRING: {
            struct object_string* s = as_string(obj);
            vfree(s->data);
            break;
        }
        case OBJECT_FUNCTION: {
            struct object_function* f = as_function(obj);
            free_bc_chunk(&f->bc);
            break;
        }
        case OBJECT_NATIVE: {
            break;
        }
        default:
            assert(false && "Unknown object type");
    }
    vfree(obj);
}

void free_value(struct value* val) {
    if (val->type == VAL_OBJECT) {
        free_object(val->object);
    }
}

u32 value_hash(struct value v) {
    u8* p;
    size_t len;
    switch (v.type) {
        case VAL_INT:
            p = (u8*)&v.integer;
            len = sizeof(v.integer);
            break;
        case VAL_BOOL:
            p = (u8*)&v.boolean;
            len = sizeof(v.boolean);
            break;
        case VAL_DOUBLE:
            p = (u8*)&v.double_num;
            len = sizeof(v.double_num);
            break;
        case VAL_OBJECT:
            // TODO: should be able to use the below pointer hash when strings are interned
            switch(v.object->type) {
                case OBJECT_STRING: {
                    struct object_string* s = as_string(v.object);
                    return s->hash;
                }
                default:
                    p = (u8*)&v.object;
                    len = sizeof(v.object);
            }
            break;
        case VAL_NONE:
            len = 0;
            break;
    }
    u32 hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
      hash ^= p[i];
      hash *= 16777619;
    }
    return hash;
}

#define VALUE_COMPARE(fun_name, operator) \
bool fun_name(struct value v1, struct value v2) {  \
    if (v1.type != v2.type) {  \
        return false;  \
    }  \
    switch (v1.type) {  \
        case VAL_INT:  \
            return v1.integer operator v2.integer;  \
        case VAL_BOOL:  \
            return v1.boolean operator v2.boolean;  \
        case VAL_DOUBLE:  \
            return v1.double_num operator v2.double_num;  \
        case VAL_NONE:  \
            return false;  \
        default:  \
            fprintf(stderr, "Unsupported type for <\n");  \
            exit(-1);  \
    }  \
}

VALUE_COMPARE(value_less, <)
VALUE_COMPARE(value_lesseq, <=)
VALUE_COMPARE(value_greater, >)
VALUE_COMPARE(value_greatereq, >=)

#undef VALUE_COMPARE

bool value_eq(struct value v1, struct value v2) {
    if (v1.type != v2.type) {
        return false;
    }
    switch (v1.type) {
        case VAL_INT:
            return v1.integer == v2.integer;
        case VAL_BOOL:
            return v1.boolean == v2.boolean;
        case VAL_DOUBLE:
            return v1.double_num == v2.double_num;
        case VAL_OBJECT:
            switch (v1.object->type) {
                case OBJECT_STRING: {
                    // TODO: use the below pointer comparison when strings are interned
                    struct object_string* s1 = as_string(v1.object);
                    struct object_string* s2 = as_string(v2.object);
                    if (s1->size != s2->size)
                        return false;
                    if (s1->hash != s2->hash)
                        return false;
                    return memcmp(s1->data, s2->data, s1->size) == 0;
                }
                default:
                    // Equal means "the same object" in the shallow sense,
                    // not structurally equal
                    return v1.object == v2.object;
            }
        case VAL_NONE:
            return true;
    }
    UNREACHABLE();
}
