#pragma once
#include "ast.h"
#include "src/memory/arena_alloc.h"

struct stmt* compile(const char* source, struct ArenaAllocator* alloc);
