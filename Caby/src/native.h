#pragma once
/// All native functions which the VM provides.
/// Native functions should always have suffix
/// _nat.
#include <time.h>

#include "object.h"

struct value clock_nat(int arg_cnt, struct value* args);

struct value pow_nat(int arg_cnt, struct value* args);
