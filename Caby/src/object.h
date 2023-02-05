#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "memory.h"
#include "common.h"
#include "bytecode.h"

/// Forward decl
typedef struct vm_state vm_t;

enum object_type {
    OBJECT_STRING,
    OBJECT_FUNCTION,
    OBJECT_NATIVE,
    OBJECT_CLASS,
    OBJECT_INSTANCE,
};

/**
 * Represents object that will be allocated on the heap like strings,
 * functions and so on.
 */
struct object {
    struct object* next;
    enum object_type type;
    /// Internal data used for GC (ie. marked, generation and so on...)
    u8 gc_data;
};

void init_object(vm_t* vm, struct object* obj, enum object_type type);

struct object_string {
    struct object object;
    /// Length of the string WITHOUT zero terminator.
    u64 size;
    u32 hash;
    /// Contains zero terminated string.
    char* data;
};

struct object_function {
    struct object object;
    u8 arity;
    u16 locals;
    struct bc_chunk bc;
    /// Index to constant pool
    u32 name;
};

typedef struct value (*native_fn_t)(int arg_cnt, struct value* args);

struct object_native {
    struct object object;
    native_fn_t function;
};

enum value_type {
    VAL_INT,
    VAL_BOOL,
    VAL_DOUBLE,
    VAL_OBJECT,
    VAL_NONE,
};

/**
 * Basically POD (data types + pointers to objects) values which are
 * expected to reside on VM operand stack.
 */
struct value {
    enum value_type type;
    union {
        int integer;
        bool boolean;
        double double_num;
        struct object* object;
    };
};

#define NEW_INT(VAL) (struct value){.type = VAL_INT, .integer = (VAL)}
#define NEW_BOOL(VAL) (struct value){.type = VAL_BOOL, .boolean = (VAL)}
#define NEW_DOUBLE(VAL) (struct value){.type = VAL_DOUBLE, .double_num = (VAL)}
#define NEW_OBJECT(VAL) (struct value){.type = VAL_OBJECT, .object = (struct object*)(VAL)}
#define NEW_NONE() (struct value){.type = VAL_NONE}

/// To improve clarity of some functions
#define AS_OBJECT(VAL) NEW_OBJECT(VAL)

/*
 * Following functions serve as constructors, converters and checkers
 * for given types. Functions with postfix _s are considered safe
 * in that they always check if the given value is the type
 * that it is casting to. If not, the function returns NULL
 * pointer.
 *
 * TODO: Consider changing those simple functions to macros,
 *       if it is worth it (it probably isn't, measure this).
 */
bool is_object_type(struct value* val, enum object_type type);

void free_object(struct object* obj);

/// If the value is object then frees all dynamically allocated memory by it,
/// otherwise it does nothing. It uses the VM internal free.
void free_value(struct value* val);

struct object* to_object(struct value val);

struct object* to_object_s(struct value val);

/// Returns new object string with contents of 'str', the string is copied.
struct object_string* new_string(vm_t* vm, const char* str);

/// Takes ownership of str, which is zero terminated string.
/// len is the length of the string without zero terminator.
struct object_string* new_string_move(vm_t* vm, char* str, u32 len);

struct object_string* as_string(struct object* object);

struct object_string* as_string_s(struct object* object);

/// Returns new function object, takes ownership of 'name'.
struct object_function* new_function(vm_t* vm, u8 arity, u16 locals, struct bc_chunk c, u32 name);

struct object_function* as_function(struct object* object);

struct object_function* as_function_s(struct object* object);

struct object_native* new_native(vm_t* vm, native_fn_t fun);

struct object_native* as_native(struct object* object);

struct object_native* as_native_s(struct object* object);

// Computes hash of a value.
// (Hashing of strings is separate, this hashes the pointer to string.)
u32 value_hash(struct value v);

// Compares two values for equality
bool value_eq(struct value v1, struct value v2);

bool value_less(struct value v1, struct value v2);
bool value_lesseq(struct value v1, struct value v2);
bool value_greater(struct value v1, struct value v2);
bool value_greatereq(struct value v1, struct value v2);
