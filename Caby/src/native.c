#include "native.h"
#include "object.h"
#include "class.h"
#include "src/vm.h"

#include <math.h>

struct value clock_nat(vm_t* vm, int arg_cnt, struct value* args) {
    if (arg_cnt != 0) {
        fprintf(stderr, "Wrong number of arguments!");
        exit(1);
    }
    return NEW_DOUBLE((double)clock() / CLOCKS_PER_SEC);
}

// Converts value of type int or double to double,
// otherwise causes runtime error.
static double val_to_double(struct value v) {
    switch (v.type) {
        case VAL_INT:
            return v.integer;
        case VAL_DOUBLE:
            return v.double_num;
        default:
            fprintf(stderr, "Expected int or double in pow function\n");
            exit(1);
    }
}

struct value pow_nat(vm_t* vm, int arg_cnt, struct value* args) {
    if (arg_cnt != 2) {
        fprintf(stderr, "Wrong number of arguments!");
        exit(1);
    }
    double base = val_to_double(args[1]);
    double exponent = val_to_double(args[0]);
    return NEW_DOUBLE(pow(base, exponent));
}

struct value print_nat(vm_t* vm, int arg_cnt, struct value* args) {
    if (arg_cnt < 1) {
        runtime_error(vm, "Wrong number of arguments!");
        exit(1);
    }
    struct object_string* obj = as_string((args++)->object);
    arg_cnt -= 1;
    for (const char* c = obj->data; *c != '\0'; ++c) {
        if (c[0] == '{' && c[1] == '}') {
            if (arg_cnt == 0) {
                runtime_error(vm, "There are more '{}' than arguments");
                exit(1);
            }

            switch (args->type) {
                case VAL_INT:
                    printf("%d", args->integer);
                    break;
                case VAL_BOOL:
                    fputs(args->boolean ? "true" : "false", stdout);
                    break;
                case VAL_DOUBLE:
                    printf("%f", args->double_num);
                    break;
                case VAL_NONE:
                    printf("none");
                    break;
                case VAL_OBJECT: {
                    switch (args->object->type) {
                        case OBJECT_STRING:
                            fputs(as_string(args->object)->data, stdout);
                            break;
                        case OBJECT_CLASS: {
                            u32 name_idx = as_class(args->object)->name;
                            const char* name = as_string(vm->const_pool.data[name_idx])->data;
                            printf("<class object '%s' at %p", name, &obj->object);
                            break;
                        }
                        case OBJECT_INSTANCE: {
                            printf("<class instance at %p>", &args->object);
                            break;
                        }
                        default:
                            runtime_error(vm, "Can't print this type");
                            exit(1);
                    }
                    break;
                }
                default:
                    UNREACHABLE();
            }

            arg_cnt -= 1;
            c += 1; // skip the {}
        } else if (*c == '\\' && c[1] != '\0') { // Escape sequence
            c += 1;
            if (*c == 'n') {
                puts("");
            }
        } else { // Normal char
            putc(*c, stdout);
        }
    }

    return NEW_NONE();
}
