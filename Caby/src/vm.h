#pragma once

#include "bytecode.h"

struct vm_state {
    struct bc_chunk* chunk;
    u8* ip;
    struct constant_pool const_pool;
};

void init_vm_state(struct vm_state* vm);

void free_vm_state(struct vm_state* vm);

int interpret(struct vm_state* vm, struct bc_chunk* chunk);
