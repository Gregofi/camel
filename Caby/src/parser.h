#pragma once
#include "ast.h"
#include "src/memory/arena_alloc.h"

struct stmt* parse(const char* source, struct ArenaAllocator* alloc, const char* sourcefile_opt);
