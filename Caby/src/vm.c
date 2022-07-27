#include "vm.h"
#include "common.h"

#include <stdbool.h>
#include <stdio.h>

void init_vm_state(struct vm_state* vm) {
    init_constant_pool(&vm->const_pool);
    vm->chunk = NULL;
}

void free_vm_state(struct vm_state* vm) {
    init_vm_state(vm);
}

static int run(struct vm_state* vm) {
#define READ_BYTE() (*vm->ip++)
    u8 ins;

    while (true) {
        ins = READ_BYTE();
        switch (ins) {
            case OP_RETURN:
                return 0;
            default:
                fprintf(stderr, "Unknown error!\n");
        }
    }

#undef READ_BYTE
}

int execute(struct vm_state* vm, struct bc_chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->data;
    return run(vm);
}


