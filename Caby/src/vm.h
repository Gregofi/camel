#pragma once

#include "bytecode.h"
#include "object.h"
#include "hashtable.h"
#include "native.h"
#include "gc.h"

#define FRAME_DEPTH 128
#define GC_HEAP_GROW_FACTOR 2

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

typedef struct vm_state {
    struct call_frame frames[FRAME_DEPTH];
    u16 frame_len;
    u8* ip;

    struct constant_pool const_pool;

    struct value* op_stack;
    size_t stack_len;
    size_t stack_cap;

    struct table globals;

    struct value* locals;

    /// Linked list of all objects in a program
    struct object* objects;

    struct gc_state gc;
} vm_t;

void init_vm_state(vm_t* vm);

void free_vm_state(vm_t* vm);

int interpret(vm_t* vm, u32 ep);

void push(vm_t* vm, struct value val);

struct value pop(vm_t* vm);

/// Returns value that is 'p' behind the top.
/// p = 0 returns top, p = 1 returns one behind top...
struct value peek(vm_t* vm, size_t p);
