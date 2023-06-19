#pragma once
/// All native functions which the VM provides.
/// Native functions should always have suffix
/// _nat.
/// Because of how are the arguments stored on the
/// stack (first is on top, etc...) you need to
/// access them accordingly, so first argument
/// is at args[arg_cnt - 1], second at args[arg_cnt - 2]
/// and so on...
#include <time.h>

#include "object.h"

struct value clock_nat(vm_t* vm, int arg_cnt, struct value* args);

struct value pow_nat(vm_t* vm, int arg_cnt, struct value* args);

struct value print_nat(vm_t* vm, int arg_cnt, struct value* args);
