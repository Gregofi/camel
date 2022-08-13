#pragma once

#include "bytecode.h"
#include "object.h"
#include "hashtable.h"

#define FRAME_DEPTH 128

enum interpret_result {
    INTERPRET_CONTINUE,
    INTERPRET_ERROR,
    INTERPRET_RETURN,
};

struct call_frame {
    struct object_function* function;
    /// Address to return to when function ends.
    u8* ret;
    /// Points to the beginning of the part of locals array that
    /// belongs to this function.
    struct value* slots;
};

struct vm_state {
    struct call_frame frames[FRAME_DEPTH];
    u16 frame_index;
    u8* ip;

    struct constant_pool const_pool;

    struct value* op_stack;
    size_t stack_len;
    size_t stack_cap;

    struct table globals;

    struct value* locals;
};

void init_vm_state(struct vm_state* vm);

void free_vm_state(struct vm_state* vm);

int interpret(struct vm_state* vm);

void push(struct vm_state* vm, struct value val);

struct value pop(struct vm_state* vm);

/// Returns value that is 'p' behind the top.
/// p = 0 returns top, p = 1 returns one behind top...
struct value peek(struct vm_state* vm, size_t p);
