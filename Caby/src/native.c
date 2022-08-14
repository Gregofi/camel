#include "native.h"
#include "object.h"

#include <math.h>

struct value clock_nat(int arg_cnt, struct value* args) {
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

struct value pow_nat(int arg_cnt, struct value* args) {
    if (arg_cnt != 2) {
        fprintf(stderr, "Wrong number of arguments!");
        exit(1);
    }
    double base = val_to_double(args[1]);
    double exponent = val_to_double(args[0]);
    return NEW_DOUBLE(pow(base, exponent));
}
