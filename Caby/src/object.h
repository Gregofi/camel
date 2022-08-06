#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "memory.h"
#include "common.h"
#include "bytecode.h"

enum object_type {
    OBJECT_STRING,
    OBJECT_FUNCTION,
};

/**
 * Represents object that will be allocated on the heap like strings,
 * functions and so on.
 */
struct object {
    enum object_type type;
};

struct object_string {
    struct object object;
    /// Length of the string WITHOUT zero terminator.
    uint64_t size;
    u32 hash;
    /// Contains zero terminated string.
    char* data;
};

struct object_function {
    struct object object;
    uint8_t arity;
    struct bc_chunk bc;
    /// Index to constant pool
    u32 name;
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

#define NEW_INT(val) (struct value){.type = VAL_INT, .integer = (val)}
#define NEW_BOOL(val) (struct value){.type = VAL_BOOL, .boolean = (val)}
#define NEW_DOUBLE(val) (struct value){.type = VAL_DOUBLE, .double_num = (val)}
#define NEW_OBJECT(val) (struct value){.type = VAL_OBJECT, .object = val}
#define NEW_NONE() (struct value){.type = VAL_NONE}

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

struct object* to_object(struct value val);

struct object* to_object_s(struct value val);

/// Returns new object string with contents of 'str', the string is copied.
struct object_string* new_string(const char* str);

/// Takes ownership of str, which is zero terminated string.
/// len is the length of the string without zero terminator.
struct object_string* new_string_move(char* str, u32 len);

struct object_string* as_string(struct object* object);

struct object_string* as_string_s(struct object* object);

/// Returns new function object, takes ownership of 'name'.
struct object_function* new_function(u8 arity, struct bc_chunk c, u32 name);

struct object_function* as_function(struct object* object);

struct object_function* as_function_s(struct object* object);
