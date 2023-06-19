#pragma once
#include "vm.h"
#include "ast.h"

vm_t compile(struct stmt* s, u32* ep);
