#pragma once
#include "src/common.h"
#include <stdlib.h>

#define GALLOC_INIT_SIZE 4 * 1024 * 1024 // 4MB

struct ArenaAllocator {
    void* mempool;
    size_t size;
    size_t taken;
};

void* gmalloc(struct ArenaAllocator* heap, u64 size, u64 align);
struct ArenaAllocator arena_init();
/// Extend the last allocated block.
void extend(struct ArenaAllocator* heap, u64 size, u64 align);
void arena_done(struct ArenaAllocator* heap);
