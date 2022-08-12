#pragma once

#include "bytecode.h"
#include "object.h"
#include "hashtable.h"

enum interpret_result {
    INTERPRET_CONTINUE,
    INTERPRET_ERROR,
    INTERPRET_RETURN,
};

struct call_frame {
    struct object_function* function;
    u8* ip;
    /// Points to the beginning of the part of operand stack
    /// which is used by this function. This is used for
    /// local variable access, since they live on the stack.
    struct value* slots;
};

struct vm_state {
    /// VM does not own bytecode chunk
    struct bc_chunk* chunk;
    u8* ip;

    struct constant_pool const_pool;

    struct value* op_stack;
    size_t stack_len;
    size_t stack_cap;

    struct call_frame* frames;

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
