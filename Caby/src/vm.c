#include "vm.h"
#include "bytecode.h"
#include "common.h"
#include "hashtable.h"
#include "object.h"
#include "dissasembler.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

void init_vm_state(struct vm_state* vm) {
    init_constant_pool(&vm->const_pool);
    init_table(&vm->globals);
    vm->locals = malloc(sizeof(*vm->locals) * (1 << 16));
    vm->op_stack = NULL;
    memset(vm->frames, 0, sizeof(vm->frames));
    vm->frame_index = 0;
    vm->stack_cap = 0;
    vm->stack_len = 0;
}

static void push_frame(struct vm_state* vm, struct object_function* f) {
    assert(vm->frame_index > 0);
    struct call_frame* new_frame = &vm->frames[vm->frame_index];
    struct call_frame* previous  = &vm->frames[vm->frame_index - 1];
    new_frame->function = f;
    new_frame->slots = previous->slots + previous->function->locals;
    new_frame->ret = vm->ip;
    vm->ip = new_frame->function->bc.data;
    vm->frame_index += 1;
}

static void pop_frame(struct vm_state* vm) {
    assert(vm->frame_index > 0);
    struct call_frame* curr_frame = &vm->frames[vm->frame_index];
    vm->ip = curr_frame->ret;
    vm->frame_index -= 1;
}

void free_vm_state(struct vm_state* vm) {
    free_constant_pool(&vm->const_pool);
    free_table(&vm->globals);
    free(vm->locals);
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

static struct object_string* pop_string(struct vm_state* vm) {
    struct value v = pop(vm);
    if (v.type != VAL_OBJECT || v.object->type != OBJECT_STRING) {
        fprintf(stderr, "Expected string on top of stack.\n");
        exit(1);
    }
    return as_string(v.object);
}

// =========== Interpreting functions ===========

#define READ_IP() (*vm->ip++)

/// Returns new 'struct value' containing string object which is contatenation of o1 and o2
struct value interpret_string_concat(struct object* o1, struct object* o2) {
    struct object_string* str1 = as_string(o1);
    struct object_string* str2 = as_string(o2);

    u32 size = str1->size + str2->size;
    char* new_char = vmalloc(size + 1);
    strncpy(new_char, str1->data, str1->size);
    strcpy(new_char + str1->size, str2->data);

    return NEW_OBJECT((struct object*)new_string_move(new_char, size));
}

void interpret_print(struct vm_state* vm) {
    u8 arg_cnt = READ_IP() - 1;

    struct value v = pop(vm);
    if (v.type != VAL_OBJECT || v.object->type != OBJECT_STRING) {
        fprintf(stderr, "First 'print' argument must be a string.\n");
        exit(-1);
    }
    struct object_string* obj = as_string(v.object);

    // TODO: Maybe buffer the output so that if error occurs we don't print it halfway.
    for (const char* c = obj->data; *c != '\0'; ++c) {
        if (*c == '{' && c[1] != '\0' && c[1] == '}') {
            if (arg_cnt == 0) {
                fprintf(stderr, "There are more '{}' than arguments.\n");
                exit(-1);
            }
            arg_cnt -= 1;
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
                    break;
                case VAL_NONE:
                    printf("none");
                    break;
                case VAL_OBJECT: {
                    switch (v.object->type) {
                        case OBJECT_STRING:
                            fputs(as_string(v.object)->data, stdout);
                            break;
                        default:
                            fprintf(stderr, "Can't print this type.\n");
                            exit(-1);
                    }
                    break;
                }
                default:
                    UNREACHABLE();
            }
        } else if (*c == '\\' &&c[1] != '\0') { // Escape sequence
            c += 1;
            if (*c == 'n') {
                puts("");
            }
        } else { // Normal char
            putc(*c, stdout);
        }
    }
    if (arg_cnt != 0) {
        fprintf(stderr, "There are more arguments than '{}'.\n");
        exit(-1);
    }

    push(vm, NEW_NONE());
}

bool interpret_eq(struct vm_state* vm) {
    struct value v1 = pop(vm);
    struct value v2 = pop(vm);
    bool eq = false;
    if (v1.type == v2.type) {
        switch (v1.type) {
            case VAL_INT:
                eq = v1.integer == v2.integer;
                break;
            case VAL_BOOL:
                eq = v1.boolean == v2.boolean;
                break;
            case VAL_DOUBLE:
                eq = v1.double_num == v2.double_num;
                break;
            case VAL_OBJECT:
                switch(v1.object->type) {
                    case OBJECT_STRING:
                        // TODO: Maybe compare hashes?
                        eq = strcmp(as_string(v1.object)->data,
                                    as_string(v2.object)->data) == 0;
                        break;
                    case OBJECT_FUNCTION: {
                        NOT_IMPLEMENTED();
                        break;
                    }
                }
                break;
            case VAL_NONE:
                eq = true;
                break;
        }
    }
    return eq;
}

static enum interpret_result interpret_ins(struct vm_state* vm, u8 ins) {
    switch (ins) {
        case OP_RETURN: {
            if (vm->frame_index > 0) {
                pop_frame(vm);
            } else {
                return INTERPRET_RETURN;
            }
            break;
        }
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
        case OP_PUSH_LITERAL: {
            u32 idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct value vobj = NEW_OBJECT(vm->const_pool.data[idx]);
            push(vm, vobj);
            break;
        }
        case OP_PUSH_NONE: {
            struct value none = NEW_NONE();
            push(vm, none);
            break;
        }
        case OP_PUSH_BOOL: {
            struct value boolean = NEW_BOOL(READ_IP());
            push(vm, boolean);
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
        case OP_EQ: {
            bool res = interpret_eq(vm);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_INEG: {
            struct value v = pop(vm);
            if (v.type == VAL_INT) {
                push(vm, NEW_INT(-v.integer));
            } else if (v.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(-v.integer));
            } else {
                fprintf(stderr, "Incopatible type for operator unary '-'\n");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_DROP:
            pop(vm);
            break;
        case OP_DROPN:
            vm->stack_len -= *vm->ip++;
            break;
        case OP_JMP:
            vm->ip = &vm->frames[vm->frame_index].function->bc.data[READ_4BYTES_BE(vm->ip)];
            break;
        case OP_BRANCH_FALSE:
            fallthrough;
        case OP_BRANCH: {
            struct value val = pop(vm);
            if (val.type != VAL_BOOL) {
                fprintf(stderr, "Expected type 'bool' in if condition");
                exit(-1);
            }
            if ((ins == OP_BRANCH && val.boolean) || (ins == OP_BRANCH_FALSE && !val.boolean)) {
                u32 dest = READ_4BYTES_BE(vm->ip);
                vm->ip = &vm->frames[vm->frame_index].function->bc.data[dest];
            } else {
                vm->ip += 4;
            }
            break;
        }
        case OP_VAL_GLOBAL:
        fallthrough;
        case OP_VAR_GLOBAL: {
            struct value val = pop(vm);
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            bool new_v = table_set(&vm->globals, name, val);
            if (!new_v) {
                fprintf(stderr, "Error: Variable '%s' is already defined.\n", name->data);
                exit(6);
            }
            break;
        }
        case OP_GET_GLOBAL: {
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            struct value val;
            if (!table_get(&vm->globals, name, &val)) {
                fprintf(stderr, "Error: Access to undefined variable '%s'.\n", name->data);
                exit(6);
            }
            push(vm, val);
            break;
        }
        case OP_SET_GLOBAL: {
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            struct value v = pop(vm);
            if (table_set(&vm->globals, name, v)) {
                fprintf(stderr, "Global variable '%s' is not defined!\n", name->data);
                exit(1);
            }
            break;
        }
        case OP_GET_LOCAL: {
            u16 frame_idx = READ_2BYTES_BE(vm->ip);
            vm->ip += 2;
            struct value v = vm->locals[frame_idx];
            push(vm, v);
            break;
        }
        case OP_SET_LOCAL: {
            u16 frame_idx = READ_2BYTES_BE(vm->ip);
            vm->ip += 2;
            struct value v = pop(vm);
            vm->locals[frame_idx] = v;
            break;
        }
        case OP_CALL_FUNC: {
            struct value v = pop(vm);
            if (v.type == VAL_OBJECT && v.object->type == OBJECT_FUNCTION) {
                struct object_function* f = as_function(v.object);
                u8 arity = READ_IP();
                if (arity != f->arity) {
                    fprintf(stderr, "Got '%d' arguments, expected '%d'", arity, f->arity);
                    return INTERPRET_ERROR;
                }
                push_frame(vm, f);
            } else {
                fprintf(stderr, "Only functions can be called\n");
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
            fprintf(stderr, "Error, halting\n");
            exit(-1);
        } else if (res == INTERPRET_RETURN) {
            return 0;
        }
    }
}

// TODO: Maybe this guy shouldn't receive vm state at all and rather
//       get constant pool, globals and entry point.
int interpret(struct vm_state* vm) {
    return run(vm);
}

#undef READ_IP
