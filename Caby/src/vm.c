#include "vm.h"
#include "bytecode.h"
#include "common.h"
#include "object.h"
#include "dissasembler.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

void init_vm_state(struct vm_state* vm) {
    init_constant_pool(&vm->const_pool);
    vm->chunk = NULL;
    vm->op_stack = NULL;
    vm->frames = NULL;
    vm->stack_cap = 0;
    vm->stack_len = 0;
}

void free_vm_state(struct vm_state* vm) {
    free_constant_pool(&vm->const_pool);
    init_vm_state(vm);
}

void push(struct vm_state* vm, struct value val) {
    vm->op_stack = handle_capacity(vm->op_stack, vm->stack_len,
                                     &vm->stack_cap, sizeof(*vm->op_stack));

    vm->op_stack[vm->stack_len++] = val;
}

struct value pop(struct vm_state* vm) {
    assert(vm->stack_len > 0);
    return vm->op_stack[--vm->stack_len];
}

struct value peek(struct vm_state* vm, size_t p) {
    return vm->op_stack[vm->stack_len - p];
}

// =========== Interpreting functions ===========

#define READ_IP() (*vm->ip++)

/// Returns new 'struct value' containing string object which is contatenation of o1 and o2
struct value interpret_string_concat(struct object* o1, struct object* o2) {
    struct object_string* str1 = as_string(o1);
    struct object_string* str2 = as_string(o2);

    struct object_string* new_str = malloc(sizeof(*new_str)
                                                + str1->size
                                                + str2->size + 1);
    strncpy(new_str->data, str1->data, str1->size);
    strcpy(new_str->data + str1->size, str2->data);
    return (struct value){.type = VAL_OBJECT, .object = (struct object*)new_str };
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
            c += 1;
            struct value v = pop(vm);
            switch (v.type) {
                case VAL_INT:
                    printf("%d", v.integer);
                    break;
                case VAL_BOOL:
                    fputs(v.boolean ? "true" : "false", stdout);
                    break;
                case VAL_DOUBLE:
                    printf("%f", v.double_num);
                case VAL_OBJECT: {
                    switch (v.object->type) {
                        case OBJECT_STRING:
                            fputs(as_string(v.object)->data, stdout);
                        default:
                            fprintf(stderr, "Can't print this type.\n");
                            exit(-1);
                    }
                }
                default:
                    UNREACHABLE();
            }
        }
        else {
            putc(*c, stdout);
        }
    }
}

static enum interpret_result interpret_ins(struct vm_state* vm, u8 ins) {
    switch (ins) {
        case OP_RETURN:
            return INTERPRET_RETURN;
        case OP_PRINT:
            interpret_print(vm);
            break;
        case OP_PUSH_SHORT: {
            i16 val = READ_2BYTES_BE(vm->ip);
            vm->ip += 2;
            push(vm, NEW_INT(val));
            break;
        }
        case OP_PUSH_INT: {
            int val = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            push(vm, NEW_INT(val));
            break;
        }
        case OP_IADD: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                push(vm, NEW_INT(v1.integer + v2.integer));
            } else if (v1.type == VAL_DOUBLE && v2.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(v1.double_num + v2.double_num));
            } else if (v1.type == VAL_OBJECT && v2.type == VAL_OBJECT
                    && v1.object->type == OBJECT_STRING
                    && v2.object->type == OBJECT_STRING) {
                push(vm, interpret_string_concat(v1.object, v2.object));
            } else {
                fprintf(stderr, "Incopatible types for operator '+'\n");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_ISUB: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                push(vm, NEW_INT(v1.integer - v2.integer));
            } else if (v1.type == VAL_DOUBLE && v2.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(v1.double_num - v2.double_num));
            } else {
                fprintf(stderr, "Incopatible types for operator '-'\n");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_IMUL: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                push(vm, NEW_INT(v1.integer * v2.integer));
            } else if (v1.type == VAL_DOUBLE && v2.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(v1.double_num * v2.double_num));
            // TODO: List and string multiplication
            } else {
                fprintf(stderr, "Incopatible types for operator '-'\n");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_IDIV: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                if (v2.integer == 0) {
                    fprintf(stderr, "Division by zero error");
                }
                push(vm, NEW_INT(v1.integer / v2.integer));
            } else if (v1.type == VAL_DOUBLE && v2.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(v1.double_num / v2.double_num));
            } else {
                fprintf(stderr, "Incopatible types for operator '-'\n");
                return INTERPRET_ERROR;
            }
            break;
        }
        default:
            fprintf(stderr, "Unknown instruction 0x%x!\n", ins);
    }
    return INTERPRET_CONTINUE;
}

static int run(struct vm_state* vm) {
    u8 ins;
    while (true) {
        ins = READ_IP();
        enum interpret_result res = interpret_ins(vm, ins);
        if (res == INTERPRET_ERROR) {
            fprintf(stderr, "Error, halting");
            exit(-1);
        } else if (res == INTERPRET_RETURN) {
            return 0;
        }
    }
}

// TODO: Maybe this guy shoudln't receive vm state at all and rather
//       get constant pool, globals and entry point.
int interpret(struct vm_state* vm) {
    vm->ip = vm->chunk->data;
    return run(vm);
}

#undef READ_IP
