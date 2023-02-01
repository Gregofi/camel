#include "vm.h"
#include "error.h"
#include "gc.h"
#include "bytecode.h"
#include "common.h"
#include "hashtable.h"
#include "object.h"
#include "class.h"
#include "dissasembler.h"
#include "native.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#ifdef __DEBUG__
    #define DUMP_INS(ins) do {dissasemble_instruction(stderr, ins);fprintf(stderr, "\n");} while(false)
#else
    #define DUMP_INS(ins)
#endif

static void disassemble_stack(vm_t* vm) {
    for (size_t i = 0; i < vm->stack_len; ++i) {
        fprintf(stderr, "[");
        disassemble_value(stderr, vm->op_stack[i]);
        fprintf(stderr, "]");
    }
    fprintf(stderr, "\n");
}

#ifdef __DEBUG__
    #define DUMP_STACK(vm) do { disassemble_stack(vm); } while (false)
#else
    #define DUMP_STACK(vm)
#endif

struct object_function* get_current_fun(vm_t* vm) {
    return vm->frames[vm->frame_len - 1].function;
}

static void runtime_error(vm_t* vm, const char* str, ...) {
    va_list args;
    va_start(args, str);
    struct object_function* f = get_current_fun(vm);
    size_t loc_idx = range_between(f->bc.data,  vm->ip) - 1;
    struct loc loc = f->bc.location[loc_idx];
    print_error(vm->filename, loc, str, args);
    va_end(args);
}

static void def_native(vm_t* vm, const char* name, native_fn_t fun) {
    push(vm, NEW_OBJECT(new_string(vm, name)));
    push(vm, NEW_OBJECT(new_native(vm, fun)));
    table_set(&vm->globals, vm->op_stack[0], vm->op_stack[1]);
    pop(vm);
    pop(vm);
}

void init_vm_state(vm_t* vm) {
    init_constant_pool(&vm->const_pool);
    init_table(&vm->globals);
    vm->locals = NULL;
    vm->op_stack = NULL;
    memset(vm->frames, 0, sizeof(vm->frames));
    vm->frame_len = 0;
    vm->stack_cap = 0;
    vm->stack_len = 0;
    vm->objects = NULL;
    init_gc(&vm->gc);
    vm->filename = NULL;
}

void alloc_frames(vm_t* vm) {
    vm->locals = malloc(sizeof(*vm->locals) * (1 << 16));
}

static void push_frame(vm_t* vm, struct object_function* f) {
    assert(vm->frame_len > 0);
    struct call_frame* new_frame = &vm->frames[vm->frame_len];
    struct call_frame* previous  = &vm->frames[vm->frame_len - 1];
    new_frame->function = f;
    new_frame->slots = previous->slots + previous->function->locals;
    new_frame->ret = vm->ip;
    vm->ip = new_frame->function->bc.data;
    vm->frame_len += 1;
}

static void pop_frame(vm_t* vm) {
    assert(vm->frame_len > 0);
    struct call_frame* curr_frame = &vm->frames[vm->frame_len - 1];
    vm->ip = curr_frame->ret;
    vm->frame_len -= 1;
}

/// Frees up structures owned by vm state.
/// Does not free constant pool, since that
/// is not owned.
void free_vm_state(vm_t* vm) {
    // Free all objects in VM heap
    while (vm->objects) {
        struct object* to_free = vm->objects;
        vm->objects = vm->objects->next;
        free_object(to_free);
    }
    free_constant_pool(&vm->const_pool);
    free_table(&vm->globals);
    free(vm->locals);
    free(vm->op_stack);
    free_gc(&vm->gc);
    init_vm_state(vm);
}

void push(vm_t* vm, struct value val) {
    vm->op_stack = handle_capacity(vm->op_stack, vm->stack_len,
                                     &vm->stack_cap, sizeof(*vm->op_stack));

    vm->op_stack[vm->stack_len++] = val;
}

struct value pop(vm_t* vm) {
    assert(vm->stack_len > 0);
    return vm->op_stack[--vm->stack_len];
}

struct value peek(vm_t* vm, size_t p) {
    return vm->op_stack[vm->stack_len - p];
}

static struct object_string* pop_string(vm_t* vm) {
    struct value v = pop(vm);
    if (v.type != VAL_OBJECT || v.object->type != OBJECT_STRING) {
        runtime_error(vm, "Expected string on top of stack");
        exit(1);
    }
    return as_string(v.object);
}

// =========== Interpreting functions ===========

#define READ_IP() (*vm->ip++)
#define TOP_FRAME() (vm->frames[vm->frame_len - 1])
#define CURRENT_FUNCTION() (TOP_FRAME().function)

/// Returns new 'struct value' containing string object which is contatenation of o1 and o2
struct value interpret_string_concat(vm_t* vm, struct object* o1, struct object* o2) {
    struct object_string* str1 = as_string(o1);
    struct object_string* str2 = as_string(o2);

    u32 size = str1->size + str2->size;
    char* new_char = vmalloc(vm, size + 1);
    memcpy(new_char, str1->data, str1->size);
    memcpy(new_char + str1->size, str2->data, str2->size);
    new_char[size] = '\0';

    return NEW_OBJECT(new_string_move(vm, new_char, size));
}

enum interpret_result interpret_print(vm_t* vm) {
    u8 arg_cnt = READ_IP() - 1;

    struct value v = pop(vm);
    if (v.type != VAL_OBJECT || v.object->type != OBJECT_STRING) {
        runtime_error(vm, "First 'print' argument must be a string");
        return INTERPRET_ERROR;
    }
    struct object_string* obj = as_string(v.object);

    // TODO: Maybe buffer the output so that if error occurs we don't print it halfway.
    for (const char* c = obj->data; *c != '\0'; ++c) {
        if (*c == '{' && c[1] != '\0' && c[1] == '}') {
            if (arg_cnt == 0) {
                runtime_error(vm, "There are more '{}' than arguments");
                return INTERPRET_ERROR;
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
                        case OBJECT_CLASS: {
                            u32 name_idx = as_class(v.object)->name;
                            const char* name = as_string(vm->const_pool.data[name_idx])->data;
                            printf("<class object '%s' at %p", name, &obj->object);
                            break;
                        }
                        case OBJECT_INSTANCE: {
                            printf("<class instance at %p>", &v.object);
                            break;
                        }
                        default:
                            runtime_error(vm, "Can't print this type");
                            return INTERPRET_ERROR;
                    }
                    break;
                }
                default:
                    UNREACHABLE();
            }
        } else if (*c == '\\' && c[1] != '\0') { // Escape sequence
            c += 1;
            if (*c == 'n') {
                puts("");
            }
        } else { // Normal char
            putc(*c, stdout);
        }
    }
    if (arg_cnt != 0) {
        runtime_error(vm, "There are more arguments than '{}'.\n");
        return INTERPRET_ERROR;
    }

    push(vm, NEW_NONE());
    return INTERPRET_CONTINUE;
}

static enum interpret_result interpret_ins(vm_t* vm, u8 ins) {
    switch (ins) {
        case OP_RETURN: {
            if (vm->frame_len > 1) {
                pop_frame(vm);
            // Only the last global frame is remaining
            } else {
                return INTERPRET_RETURN;
            }
            break;
        }
        case OP_PRINT:
            return interpret_print(vm);
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
                push(vm, interpret_string_concat(vm, v1.object, v2.object));
            } else {
                runtime_error(vm, "Incopatible types for operator '+'");
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
                runtime_error(vm, "Incopatible types for operator '-'");
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
                runtime_error(vm, "Incopatible types for operator '*'");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_IDIV: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                if (v2.integer == 0) {
                    runtime_error(vm, "Division by zero error");
                    return INTERPRET_ERROR;
                }
                push(vm, NEW_INT(v1.integer / v2.integer));
            } else if (v1.type == VAL_DOUBLE && v2.type == VAL_DOUBLE) {
                push(vm, NEW_DOUBLE(v1.double_num / v2.double_num));
            } else {
                runtime_error(vm, "Incopatible types for operator '/'");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_IMOD: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            if (v1.type == VAL_INT && v2.type == VAL_INT) {
                if (v2.integer == 0) {
                    runtime_error(vm, "Division by zero error");
                    return INTERPRET_ERROR;
                }
                push(vm, NEW_INT(v1.integer % v2.integer));
            } else {
                runtime_error(vm, "Incopatible types for operator '\%'");
                return INTERPRET_ERROR;
            }
            break;

        }
        case OP_ILESS: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = value_less(v1, v2);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_ILESSEQ: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = value_lesseq(v1, v2);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_IGREATER: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = value_greater(v1, v2);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_IGREATEREQ: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = value_greatereq(v1, v2);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_EQ: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = value_eq(v1, v2);
            push(vm, NEW_BOOL(res));
            break;
        }
        case OP_NEQ: {
            struct value v1 = pop(vm);
            struct value v2 = pop(vm);
            bool res = !value_eq(v1, v2);
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
                runtime_error(vm, "Incopatible type for operator unary '-'");
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
            vm->ip = &CURRENT_FUNCTION()->bc.data[READ_4BYTES_BE(vm->ip)];
            break;
        case OP_BRANCH_FALSE:
        case OP_BRANCH: {
            struct value val = pop(vm);
            if (val.type != VAL_BOOL) {
                runtime_error(vm, "Expected type 'bool' in if condition");
                return INTERPRET_ERROR;
            }
            if ((ins == OP_BRANCH && val.boolean) || (ins == OP_BRANCH_FALSE && !val.boolean)) {
                u32 dest = READ_4BYTES_BE(vm->ip);
                vm->ip = &CURRENT_FUNCTION()->bc.data[dest];
            } else {
                vm->ip += 4;
            }
            break;
        }
        case OP_VAL_GLOBAL:
        case OP_VAR_GLOBAL: {
            struct value val = pop(vm);
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            struct value name_obj = NEW_OBJECT(name);
            bool new_v = table_set(&vm->globals, name_obj, val);
            if (!new_v) {
                runtime_error(vm, "Error: Variable '%s' is already defined", name->data);
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_GET_GLOBAL: {
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            struct value name_obj = NEW_OBJECT(name);
            struct value val;
            if (!table_get(&vm->globals, name_obj, &val)) {
                runtime_error(vm, "Error: Access to undefined variable '%s'.", name->data);
                return INTERPRET_ERROR;
            }
            push(vm, val);
            break;
        }
        case OP_SET_GLOBAL: {
            u32 name_idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_string* name = read_string_cp(&vm->const_pool, name_idx);
            struct value name_obj = NEW_OBJECT(name);
            struct value v = pop(vm);
            if (table_set(&vm->globals, name_obj, v)) {
                runtime_error(vm, "Global variable '%s' is not defined!", name->data);
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL: {
            u16 slot_idx = READ_2BYTES_BE(vm->ip);
            vm->ip += 2;
            struct value v = TOP_FRAME().slots[slot_idx];
            push(vm, v);
            break;
        }
        case OP_SET_LOCAL: {
            u16 frame_idx = READ_2BYTES_BE(vm->ip);
            vm->ip += 2;
            struct value v = pop(vm);
            TOP_FRAME().slots[frame_idx] = v;
            break;
        }
        case OP_CALL_FUNC: {
            struct value v = pop(vm);
            if (v.type == VAL_OBJECT) {
                u8 arity = READ_IP();
                if (v.object->type == OBJECT_FUNCTION) {
                    struct object_function* f = as_function(v.object);
                    if (arity != f->arity) {
                        runtime_error(vm, "Got '%d' arguments, expected '%d'", arity, f->arity);
                        return INTERPRET_ERROR;
                    }
                    push_frame(vm, f);
                } else if (v.object->type == OBJECT_NATIVE) {
                    struct object_native* nat = as_native(v.object);
                    DUMP_STACK(vm);
                    struct value res = nat->function(arity, vm->op_stack + vm->stack_len - arity);
                    vm->stack_len -= arity;
                    push(vm, res);
                }
            } else {
                runtime_error(vm, "Only functions can be called");
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_NEW_OBJECT: {
            u32 idx = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_instance* ins = new_instance(vm,  as_class(vm->const_pool.data[idx]));
            struct value ins_v = NEW_OBJECT(ins);
            push(vm, ins_v);
            break;
        }
        case OP_SET_MEMBER:
        case OP_GET_MEMBER: {
            u32 name = READ_4BYTES_BE(vm->ip);
            vm->ip += 4;
            struct object_instance* instance = as_instance(pop(vm).object);
            struct object_string* key = as_string(vm->const_pool.data[name]);
            struct value key_v = NEW_OBJECT(key);
            struct value val;

            if (ins == OP_GET_MEMBER) {
                bool exists = table_get(&instance->members, key_v, &val);
                if (!exists) {
                    runtime_error(vm, "The object doesn't have member '%s'", key->data);
                }
                push(vm, val);
            } else { // OP_SET_MEMBER
                struct value v = pop(vm);
                table_set(&instance->members, key_v, v);
            }

            break;
        }
        default:
            runtime_error(vm, "Unknown instruction 0x%x! Skipping...", ins);
    }
    // Some branches return when successfull (like print),
    // be careful if you add code here.
    return INTERPRET_CONTINUE;
}

static int run(vm_t* vm) {
    u8 ins;
    while (true) {
        DUMP_INS(vm->ip);
        ins = READ_IP();
        enum interpret_result res = interpret_ins(vm, ins);
        DUMP_STACK(vm);
        if (res == INTERPRET_ERROR) {
            exit(-1);
        } else if (res == INTERPRET_RETURN) {
            return 0;
        }
    }
}

int interpret(vm_t* vm, u32 ep) {
    alloc_frames(vm);

    def_native(vm, "clock", clock_nat);
    def_native(vm, "pow", pow_nat);

    struct call_frame* entry = &vm->frames[vm->frame_len++];
    entry->function = (struct object_function*)vm->const_pool.data[ep];
    // There should never be a return from global
    entry->ret = 0;
    entry->slots = vm->locals;
    vm->ip = entry->function->bc.data;

    int res = run(vm);

    return res;
}

#undef READ_IP
#undef TOP_FRAME
