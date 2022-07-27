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

void interpret_print(struct vm_state* vm) {
    u32 idx = READ_4BYTES_BE(vm->ip);

    struct object_string* obj = as_string_s(vm->const_pool.data[idx]);
    if (obj == NULL) {
        fprintf(stderr, "print argument must be a string!\n");
        exit(-1);
    }

    for (const char* c = obj->data; *c != '\0'; ++c) {
        if (*c == '{' && c[1] != '\0' && c[1] == '}') {
            NOT_IMPLEMENTED();
        }
        // TODO: Escaping
        else {
            putc(*c, stdout);
        }
    }
}

static int run(struct vm_state* vm) {
#define READ_IP() (*vm->ip++)
    u8 ins;

    while (true) {
        ins = READ_IP();
        switch (ins) {
            case OP_RETURN:
                return 0;
            case OP_PRINT:
                interpret_print(vm);
                break;
            default:
                fprintf(stderr, "Unknown error!\n");
        }
    }

#undef READ_IP
}

int interpret(struct vm_state* vm, struct bc_chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->data;
    return run(vm);
}


