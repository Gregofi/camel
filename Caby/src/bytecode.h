#pragma once

#include "common.h"
#include "object.h"

#include <stdlib.h>

enum opcode {
    OP_RETURN,
    OP_PUSH_SHORT,
    OP_PUSH_INT,
    OP_PUSH_BOOL,
    OP_PUSH_LITERAL,
    // OP_GET_LOCAL,
    // OP_SET_LOCAL,
    // OP_CALL_FUNC,
    OP_LABEL,
    OP_JMP_SHORT,
    OP_JMP,
    // OP_JMP_LONG,
    OP_BRANCH_SHORT,
    OP_BRANCH,
    // OP_BRANCH_LONG,
    OP_PRINT,
    OP_DROP,
    OP_DUP,
    // OP_GET_GLOBAL,
    // OP_SET_GLOBAL,

    OP_IADD,
    OP_ISUB,
    OP_IMUL,
    OP_IDIV,
    OP_IREM,
    OP_IAND,
    OP_IOR,
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

void init_constant_pool(struct constant_pool* cp);

void free_constant_pool(struct constant_pool* cp);

void write_constant_pool(struct constant_pool* cp, struct object* object);
