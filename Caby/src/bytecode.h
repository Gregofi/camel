#pragma once

#include "common.h"

#include <stdlib.h>

// forward decl
struct object;

enum opcode {
    OP_RETURN = 0x09,
    OP_PUSH_SHORT = 0x01,
    OP_PUSH_INT = 0x02,
    OP_PUSH_BOOL = 0x04,
    OP_PUSH_LITERAL = 0x05,
    OP_PUSH_NONE = 0x20,
    OP_GET_LOCAL = 0x06,
    OP_SET_LOCAL = 0x07,
    OP_CALL_FUNC = 0x08,
    OP_LABEL = 0x00,
    OP_JMP_SHORT = 0x0A,
    OP_JMP = 0x0B,
    // OP_JMP_LONG,
    OP_BRANCH_SHORT = 0x0D,
    OP_BRANCH = 0x0E,
    // OP_BRANCH_LONG,
    OP_BRANCH_FALSE_SHORT = 0x2D,
    OP_BRANCH_FALSE = 0x2E,
    OP_BRANCH_FALSE_LONG = 0x2F,
    OP_PRINT = 0x10,
    OP_DROP = 0x11,
    OP_DROPN = 0x25,
    OP_DUP = 0x12,
    OP_GET_GLOBAL = 0x13,
    OP_SET_GLOBAL = 0x14,
    OP_VAL_GLOBAL = 0x15,
    OP_VAR_GLOBAL = 0x16,

    OP_IADD = 0x30,
    OP_ISUB = 0x31,
    OP_IMUL = 0x32,
    OP_IDIV = 0x33,
    OP_IREM = 0x34,
    OP_IAND = 0x35,
    OP_IOR = 0x36,
    OP_EQ = 0x3b,
};

struct bc_chunk {
    u8* data;
    size_t len;
    size_t cap;
};

struct constant_pool {
    /// An array of pointers to data
    struct object** data;
    size_t len;
    size_t cap;
};

void init_bc_chunk(struct bc_chunk* c);

void free_bc_chunk(struct bc_chunk* c);

void write_byte(struct bc_chunk* c, u8 byte);

void write_word(struct bc_chunk* c, u16 word);

void write_dword(struct bc_chunk* c, u32 dword);

void init_constant_pool(struct constant_pool* cp);

void free_constant_pool(struct constant_pool* cp);

void write_constant_pool(struct constant_pool* cp, struct object* object);

struct object* read_constant_pool(struct constant_pool* cp, u32 idx);

struct object_string* read_string_cp(struct constant_pool* cp, u32 idx);

struct object_function* read_function_cp(struct constant_pool* cp, u32 idx);
